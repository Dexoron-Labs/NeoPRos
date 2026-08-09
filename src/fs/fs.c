#include "fs/fs.h"
#include "lib/string.h"
#include "kernel/console.h"

/*
 * FAT12 read-only (образ 1.44 МБ как RAM-диск).
 *
 * Геометрия BPB (типовой флоппи):
 *   bytes/sector = 512, sectors/cluster = 1, reserved = 1,
 *   FATs = 2, FAT size = 9 секторов, root = 224 записи (14 секторов),
 *   total = 2880 секторов. Но все значения читаются из BPB,
 *   так что подойдёт любой корректный FAT12-образ.
 */

/* --- атрибуты записей каталога -------------------------------------- */
#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN    0x02
#define FAT_ATTR_SYSTEM    0x04
#define FAT_ATTR_VOLUME    0x08
#define FAT_ATTR_DIR       0x10
#define FAT_ATTR_ARCHIVE   0x20

/* --- состояние диска ------------------------------------------------ */

static const uint8_t *disk_base;   /* начало образа в памяти */
static uint32_t disk_size;         /* размер образа, байт */

/* Имя модуля (cmdline). Копируется в статический буфер: исходная
 * строка живёт в списке модулей Multiboot, который битовая карта
 * pmm затирает при инициализации. */
static char disk_name[64];

/* Параметры из BPB (заполняются при инициализации) */
static uint16_t bpb_bps;           /* байт на сектор (0x0B) */
static uint8_t  bpb_spc;           /* секторов на кластер (0x0D) */
static uint16_t bpb_reserved;      /* зарезервировано секторов (0x0E) */
static uint8_t  bpb_fats;          /* число FAT (0x10) */
static uint16_t bpb_root_entries;  /* записей в корне (0x11) */
static uint16_t bpb_total;         /* всего секторов (0x13) */
static uint16_t bpb_fat_size;      /* размер FAT, секторов (0x16) */

/* Стек текущего каталога: кластеры вложенных каталогов */
static uint16_t cwd_stack[FS_MAX_DEPTH];
static uint32_t cwd_depth;

/* Число секторов, занимаемых корневым каталогом, и первый сектор данных */
static uint32_t root_dir_sectors;
static uint32_t first_data_sector;

/* --- чтение диска ---------------------------------------------------- */

/* Чтение сектора LBA в buf (bps байт). */
static int disk_read_sector(uint32_t lba, void *buf)
{
    uint64_t off = (uint64_t)lba * bpb_bps;
    if (lba >= bpb_total || off + bpb_bps > disk_size) {
        return 0;
    }
    memcpy(buf, disk_base + off, bpb_bps);
    return 1;
}

/* Запись FAT: биты кластера n (0–4095). */
static uint16_t fat_entry(uint16_t n)
{
    uint32_t off = (uint32_t)bpb_reserved * bpb_bps + n + n / 2;
    if (off + 1 >= disk_size) {
        return 0x0FFF;              /* считаем концом цепочки */
    }
    uint16_t word = disk_base[off] | (disk_base[off + 1] << 8);
    return (n & 1) ? (word >> 4) : (word & 0x0FFF);
}

/* Следующий кластер цепочки; 0x0FFF = конец. */
static uint16_t fat_next(uint16_t cluster)
{
    uint16_t e = fat_entry(cluster);
    return (e >= 0x0FF8) ? 0x0FFF : e;
}

/* LBA первого сектора кластера. */
static uint32_t cluster_sector(uint16_t cluster)
{
    return first_data_sector + (uint32_t)(cluster - 2) * bpb_spc;
}

/* --- каталоги --------------------------------------------------------- */

/* Запись каталога (32 байта). */
struct fat_dirent {
    uint8_t  name[11];       /* 8.3, uppercase, дополнено пробелами */
    uint8_t  attr;
    uint8_t  nt_res;
    uint8_t  crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t first_cluster_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t first_cluster;
    uint32_t file_size;
};

/* Перебор записей каталога. */
struct dir_iter {
    uint32_t sector;         /* текущий сектор (для корня) */
    uint16_t cluster;        /* текущий кластер (для подкаталога) */
    uint32_t offset;         /* смещение записи в секторе */
    uint32_t sectors_left;   /* оставшиеся секторы корня */
    uint8_t  sector_buf[512];
    int      in_root;
    int      done;
};

/* Итератор каталога: каталог с кластером 0 — корневой. */
static int dir_iter_next_sector(struct dir_iter *it);

static void dir_iter_init(struct dir_iter *it, uint16_t cluster)
{
    it->done = 0;
    if (cluster == 0) {
        it->in_root = 1;
        it->sector = bpb_reserved + (uint32_t)bpb_fats * bpb_fat_size;
        it->sectors_left = root_dir_sectors;
        it->cluster = 0;
    } else {
        it->in_root = 0;
        it->cluster = cluster;
        it->sector = 0;
        it->sectors_left = 0;
    }
    it->offset = 0;
    it->sector_buf[0] = 0;
    /* первый сектор каталога загружается сразу: dir_iter_next
     * перечитывает сектор только при offset >= bps */
    if (!dir_iter_next_sector(it)) {
        it->done = 1;
    }
}

/* Загрузка следующего сектора каталога; 0 = конец. */
static int dir_iter_next_sector(struct dir_iter *it)
{
    if (it->in_root) {
        if (it->sectors_left == 0) {
            return 0;
        }
        if (!disk_read_sector(it->sector, it->sector_buf)) {
            it->done = 1;
            return 0;
        }
        it->sector++;
        it->sectors_left--;
        return 1;
    }
    if (it->cluster == 0x0FFF) {
        return 0;
    }
    if (!disk_read_sector(cluster_sector(it->cluster), it->sector_buf)) {
        it->done = 1;
        return 0;
    }
    it->cluster = fat_next(it->cluster);
    return 1;
}

/* Следующая валидная запись каталога (0 = конец). */
static int dir_iter_next(struct dir_iter *it, struct fat_dirent *e)
{
    for (;;) {
        if (it->offset >= bpb_bps) {
            if (!dir_iter_next_sector(it) || it->done) {
                return 0;
            }
            it->offset = 0;
        }
        memcpy(e, it->sector_buf + it->offset, sizeof(*e));
        it->offset += 32;

        if (e->name[0] == 0x00) {
            return 0;               /* конец каталога */
        }
        if (e->name[0] == 0xE5) {
            continue;               /* удалённая запись */
        }
        if (e->attr & FAT_ATTR_VOLUME) {
            continue;               /* метка тома */
        }
        return 1;
    }
}

/* --- имена 8.3 --------------------------------------------------------- */

/* Нормализация имени в формат записи: "NAME.EXT" → "NAME    EXT".
 * Расширение — всё после последней точки. Регистр — верхний. */
static void name_to_83(const char *name, char out[11])
{
    int i;
    for (i = 0; i < 11; i++) {
        out[i] = ' ';
    }
    const char *dot = NULL;
    for (const char *p = name; *p; p++) {
        if (*p == '.') {
            dot = p;
        }
    }
    const char *base = name;
    const char *end = dot ? dot : name + strlen(name);
    int b = 0;
    for (const char *p = base; p < end && b < 8; p++) {
        out[b++] = to_upper(*p);
    }
    if (dot) {
        int e = 0;
        for (const char *p = dot + 1; *p && e < 3; p++) {
            out[8 + e++] = to_upper(*p);
        }
    }
}

/* Сравнение имени из записи каталога с запросом (регистронезависимо). */
static int name_matches(const struct fat_dirent *e, const char *name)
{
    char want[11];
    name_to_83(name, want);
    for (int i = 0; i < 11; i++) {
        if (e->name[i] != want[i]) {
            return 0;
        }
    }
    return 1;
}

/* --- поиск ------------------------------------------------------------- */

/* Поиск записи в каталоге-кластере. Возвращает 1 и заполняет e. */
static int dir_find_in(uint16_t cluster, const char *name,
                       struct fat_dirent *e)
{
    struct dir_iter it;
    dir_iter_init(&it, cluster);
    while (dir_iter_next(&it, e)) {
        if (name_matches(e, name)) {
            return 1;
        }
    }
    return 0;
}

/*
 * Поиск по пути "A/B/FILE.EXT" относительно текущего каталога.
 * Промежуточные компоненты должны быть каталогами. Последний
 * компонент ищется как есть (файл или каталог). Возвращает
 * запись в e (имя последнего компонента), а его каталог — в *dir.
 */
static int fs_find_path(const char *path, struct fat_dirent *e,
                        uint16_t *dir)
{
    uint16_t cur = (cwd_depth == 0) ? 0 : cwd_stack[cwd_depth - 1];

    if (*path == '\0') {
        return 0;
    }

    char comp[32];
    const char *p = path;
    for (;;) {
        int i = 0;
        while (*p && *p != '/' && i < (int)sizeof(comp) - 1) {
            comp[i++] = *p++;
        }
        comp[i] = '\0';
        int last = (*p == '\0');
        if (!last) {
            p++;                    /* пропустить '/' */
        }

        if (strcmp(comp, ".") == 0) {
            if (last) {
                e->first_cluster = cur;
                e->attr = FAT_ATTR_DIR;
                *dir = cur;
                return 1;
            }
            continue;
        }
        if (strcmp(comp, "..") == 0) {
            /* родитель: из стека текущего каталога не вынимаем —
             * ищем запись '..' внутри самого каталога */
            struct fat_dirent up;
            if (!dir_find_in(cur, "..", &up)) {
                return 0;
            }
            if (last) {
                e->first_cluster = up.first_cluster;
                e->attr = FAT_ATTR_DIR;
                *dir = cur;
                return 1;
            }
            cur = up.first_cluster;
            continue;
        }

        if (!dir_find_in(cur, comp, e)) {
            return 0;
        }
        if (last) {
            *dir = cur;
            return 1;
        }
        if (!(e->attr & FAT_ATTR_DIR)) {
            return 0;               /* промежуточный компонент — файл */
        }
        cur = e->first_cluster;
    }
}

/* --- чтение файла ------------------------------------------------------ */

/* Чтение до size байт файла (first_cluster, file_size) в buf. */
static uint32_t read_chain(uint16_t cluster, uint32_t file_size,
                           void *buf, uint32_t size)
{
    uint8_t *dst = buf;
    uint32_t done = 0;
    uint32_t want = (size < file_size) ? size : file_size;

    while (cluster != 0x0FFF && done < want) {
        uint32_t sector = cluster_sector(cluster);
        for (uint32_t s = 0; s < bpb_spc && done < want; s++) {
            uint8_t sec[512];
            if (!disk_read_sector(sector + s, sec)) {
                return done;
            }
            uint32_t take = want - done;
            if (take > bpb_bps) {
                take = bpb_bps;
            }
            memcpy(dst + done, sec, take);
            done += take;
        }
        cluster = fat_next(cluster);
    }
    return done;
}

/* --- публичное API ------------------------------------------------------ */

void fs_init(const struct multiboot_info *mbi)
{
    disk_base = NULL;
    disk_size = 0;
    disk_name[0] = '\0';
    cwd_depth = 0;

    /* модули: бит 3 флагов + ненулевой счётчик */
    if (!(mbi->flags & (1u << 3)) || mbi->mods_count == 0) {
        return;
    }
    const struct multiboot_module *mod =
        (const struct multiboot_module *)mbi->mods_addr;
    if (mod->end <= mod->start) {
        return;
    }

    disk_base = (const uint8_t *)mod->start;
    disk_size = mod->end - mod->start;
    if (mod->string != 0) {
        const char *src = (const char *)mod->string;
        uint32_t i;
        for (i = 0; i < sizeof(disk_name) - 1 && src[i] != '\0'; i++) {
            disk_name[i] = src[i];
        }
        disk_name[i] = '\0';
    }

    /* разбор BPB */
    bpb_bps = disk_base[0x0B] | (disk_base[0x0C] << 8);
    bpb_spc = disk_base[0x0D];
    bpb_reserved = disk_base[0x0E] | (disk_base[0x0F] << 8);
    bpb_fats = disk_base[0x10];
    bpb_root_entries = disk_base[0x11] | (disk_base[0x12] << 8);
    bpb_total = disk_base[0x13] | (disk_base[0x14] << 8);
    bpb_fat_size = disk_base[0x16] | (disk_base[0x17] << 8);

    if (bpb_bps != 512 || bpb_spc == 0 || bpb_total == 0 ||
        (uint64_t)bpb_total * bpb_bps > disk_size) {
        disk_base = NULL;           /* непохоже на FAT12 */
        disk_size = 0;
        return;
    }

    root_dir_sectors =
        ((uint32_t)bpb_root_entries * 32 + bpb_bps - 1) / bpb_bps;
    first_data_sector =
        (uint32_t)bpb_reserved + (uint32_t)bpb_fats * bpb_fat_size +
        root_dir_sectors;
}

int fs_available(void)
{
    return disk_base != NULL;
}

/* Базовый адрес образа в памяти (0, если диска нет). */
uint32_t fs_image_base(void)
{
    return (uint32_t)disk_base;
}

const char *fs_image_name(void)
{
    return disk_name;
}

uint32_t fs_image_size(void)
{
    return disk_size;
}

/* Поиск записи; e заполняется, каталог записи — в *dir. */
static int fs_find(const char *name, struct fat_dirent *e, uint16_t *dir)
{
    return fs_find_path(name, e, dir);
}

uint32_t fs_load(const char *name, void *buf, uint32_t size)
{
    if (disk_base == NULL || name == NULL || *name == '\0') {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    if (!fs_find(name, &e, &dir) || (e.attr & FAT_ATTR_DIR)) {
        return 0;
    }
    return read_chain(e.first_cluster, e.file_size, buf, size);
}

uint32_t fs_list(const char *dir, char *buf, uint32_t size)
{
    if (disk_base == NULL) {
        return 0xFFFFFFFFu;
    }

    struct fat_dirent e;
    uint16_t start;

    if (dir == NULL || *dir == '\0') {
        start = (cwd_depth == 0) ? 0 : cwd_stack[cwd_depth - 1];
    } else {
        if (!fs_find(dir, &e, &start) || !(e.attr & FAT_ATTR_DIR)) {
            return 0xFFFFFFFFu;
        }
        start = e.first_cluster;
    }

    struct dir_iter it;
    char *dst = buf;
    char *end = buf + size;
    uint32_t count = 0;

    dir_iter_init(&it, start);
    while (dir_iter_next(&it, &e)) {
        /* записи "." и ".." не показываем */
        if (e.name[0] == '.' &&
            (e.name[1] == ' ' || e.name[1] == '.')) {
            continue;
        }
        /* строка: "NAME.EXT SIZE\n" (каталог: "NAME/ SIZE\n") */
        char line[32];
        char *p = line;
        int is_dir = (e.attr & FAT_ATTR_DIR) != 0;

        int base = 7;
        while (base >= 0 && e.name[base] == ' ') {
            base--;
        }
        for (int i = 0; i <= base; i++) {
            *p++ = (char)e.name[i];
        }
        if (is_dir) {
            *p++ = '/';
        }
        int ext = 10;
        while (ext > 8 && e.name[ext] == ' ') {
            ext--;
        }
        if (ext > 8) {
            *p++ = '.';
            for (int i = 8; i <= ext; i++) {
                *p++ = (char)e.name[i];
            }
        }
        *p++ = ' ';
        uint32_t sz = is_dir ? 0u : e.file_size;
        char tmp[12];
        char *q = tmp + sizeof(tmp) - 1;
        *q = '\0';
        do {
            *--q = (char)('0' + sz % 10);
            sz /= 10;
        } while (sz != 0);
        while (*q) {
            *p++ = *q++;
        }
        *p++ = '\n';

        uint32_t len = (uint32_t)(p - line);
        if (dst + len + 1 > end) {
            return 0xFFFFFFFFu;     /* буфер мал */
        }
        memcpy(dst, line, len);
        dst += len;
        count++;
    }
    *dst = '\0';
    return count;
}

int fs_exists(const char *name)
{
    if (disk_base == NULL || name == NULL) {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    return fs_find(name, &e, &dir) ? 1 : 0;
}

uint32_t fs_size(const char *name)
{
    if (disk_base == NULL || name == NULL) {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    if (!fs_find(name, &e, &dir) || (e.attr & FAT_ATTR_DIR)) {
        return 0;
    }
    return e.file_size;
}

int fs_chdir(const char *dir)
{
    if (disk_base == NULL || dir == NULL || *dir == '\0') {
        return 0;
    }
    if (strcmp(dir, "/") == 0) {
        cwd_depth = 0;
        return 1;
    }
    if (strcmp(dir, "..") == 0) {
        if (cwd_depth == 0) {
            return 0;
        }
        cwd_depth--;
        return 1;
    }
    if (cwd_depth >= FS_MAX_DEPTH) {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir_cluster;
    if (!fs_find(dir, &e, &dir_cluster) || !(e.attr & FAT_ATTR_DIR)) {
        return 0;
    }
    cwd_stack[cwd_depth++] = e.first_cluster;
    return 1;
}

void fs_getcwd(char *buf, uint32_t size)
{
    if (size == 0) {
        return;
    }
    if (cwd_depth == 0) {
        buf[0] = '/';
        buf[1] = '\0';
        return;
    }
    char *p = buf;
    char *end = buf + size - 1;

    for (uint32_t i = 0; i < cwd_depth; i++) {
        /* имя каталога глубины i ищем в его родителе */
        uint16_t parent = (i == 0) ? 0 : cwd_stack[i - 1];
        struct dir_iter pit;
        struct fat_dirent e;
        const char *name = NULL;

        dir_iter_init(&pit, parent);
        while (dir_iter_next(&pit, &e)) {
            if (e.name[0] == '.') {
                continue;           /* записи "." и ".." */
            }
            if ((e.attr & FAT_ATTR_DIR) &&
                e.first_cluster == cwd_stack[i]) {
                name = (const char *)e.name;
                break;
            }
        }
        if (name == NULL) {
            break;
        }
        int b = 7;
        while (b >= 0 && name[b] == ' ') {
            b--;
        }
        if (p + b + 1 >= end) {
            break;
        }
        for (int j = 0; j <= b; j++) {
            *p++ = (char)name[j];
        }
        int ex = 10;
        while (ex > 8 && name[ex] == ' ') {
            ex--;
        }
        if (ex > 8 && p + 4 < end) {
            *p++ = '.';
            for (int j = 8; j <= ex; j++) {
                *p++ = (char)name[j];
            }
        }
        if (i + 1 < cwd_depth && p + 1 < end) {
            *p++ = '/';
        }
    }
    *p = '\0';
}
