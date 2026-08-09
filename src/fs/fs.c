#include "fs/fs.h"
#include "lib/string.h"
#include "kernel/console.h"
#include "kernel/rtc.h"

/*
 * FAT12 (образ 1.44 МБ как RAM-диск): чтение и запись.
 * Запись идёт прямо в память образа: изменения теряются
 * при перезагрузке (свойство RAM-диска).
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

static uint8_t *disk_base;       /* начало образа в памяти */
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

/* --- запись диска ---------------------------------------------------- */

/* Запись сектора LBA (bps байт). */
static int disk_write_sector(uint32_t lba, const void *buf)
{
    uint64_t off = (uint64_t)lba * bpb_bps;
    if (lba >= bpb_total || off + bpb_bps > disk_size) {
        return 0;
    }
    memcpy(disk_base + off, buf, bpb_bps);
    return 1;
}

/* Запись FAT-записи кластера n во все копии FAT. */
static void fat_write_entry(uint16_t n, uint16_t val)
{
    for (uint8_t fat = 0; fat < bpb_fats; fat++) {
        uint32_t off = ((uint32_t)bpb_reserved + (uint32_t)fat * bpb_fat_size)
                       * bpb_bps + n + n / 2;
        if (off + 1 >= disk_size) {
            return;
        }
        uint16_t word = disk_base[off] | (disk_base[off + 1] << 8);
        if (n & 1) {
            word = (uint16_t)((word & 0x000F) | (val << 4));
        } else {
            word = (uint16_t)((word & 0xF000) | val);
        }
        disk_base[off] = (uint8_t)word;
        disk_base[off + 1] = (uint8_t)(word >> 8);
    }
}

/* Максимальный номер кластера данных (FAT12: 0x0FF5). */
static uint16_t max_cluster(void)
{
    uint32_t clusters = ((uint32_t)bpb_total - first_data_sector) / bpb_spc;
    uint32_t maxc = 2 + clusters;
    if (maxc > 0x0FF5) {
        return 0x0FF5;
    }
    return (uint16_t)maxc;
}

/* Выделение свободного кластера (помечается как конец цепочки).
 * 0 = место закончилось. */
static uint16_t alloc_cluster(void)
{
    uint16_t maxc = max_cluster();
    for (uint16_t c = 2; c < maxc; c++) {
        if (fat_entry(c) == 0) {
            fat_write_entry(c, 0x0FFF);
            return c;
        }
    }
    return 0;
}

/* Освобождение цепочки кластеров (FAT обнуляется). */
static void free_chain(uint16_t first)
{
    uint16_t c = first;
    while (c != 0x0FFF) {
        uint16_t next = fat_next(c);
        fat_write_entry(c, 0x0000);
        c = next;
    }
}

/* Запись size байт по цепочке кластеров, расширяя её по мере нужды.
 * Если *first == 0 и size > 0 — выделяет первый кластер и отдаёт его
 * в *first. Последний кластер цепочки помечается концом. */
static int write_chain(uint16_t *first, const void *data, uint32_t size)
{
    const uint8_t *src = (const uint8_t *)data;
    uint32_t left = size;
    uint16_t c = *first;
    uint16_t prev = 0;

    while (left > 0) {
        if (c == 0) {
            c = alloc_cluster();
            if (c == 0) {
                return 0;
            }
            if (prev != 0) {
                fat_write_entry(prev, c);
            } else {
                *first = c;
            }
        }
        uint32_t sector = cluster_sector(c);
        for (uint32_t s = 0; s < bpb_spc && left > 0; s++) {
            uint8_t sec[512];
            uint32_t take = (left >= bpb_bps) ? bpb_bps : left;
            if (take == bpb_bps) {
                memcpy(sec, src, bpb_bps);
            } else {
                /* хвостовой сектор: читаем, чтобы не испортить соседей */
                disk_read_sector(sector + s, sec);
                memcpy(sec, src, take);
            }
            if (!disk_write_sector(sector + s, sec)) {
                return 0;
            }
            src += take;
            left -= take;
        }
        prev = c;
        c = fat_next(c);
        if (c == 0x0FFF && left > 0) {
            c = 0;                  /* цепочка кончилась — выделим дальше */
        }
    }
    return 1;
}

/* --- каталоги --------------------------------------------------------- */

/* Запись каталога (32 байта). packed: поля FAT не выравниваются
 * (first_cluster на 26, file_size на 28). */
struct __attribute__((packed)) fat_dirent {
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
    uint32_t last_lba;       /* LBA последнего загруженного сектора */
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
        it->last_lba = it->sector;
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
    it->last_lba = cluster_sector(it->cluster);
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
 * Расширение — всё после последней точки. Регистр — верхний.
 * Спецслучаи "." и ".." пишутся как есть (".." + пробелы). */
static void name_to_83(const char *name, char out[11])
{
    int i;
    for (i = 0; i < 11; i++) {
        out[i] = ' ';
    }
    if (name[0] == '.' &&
        (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
        out[0] = '.';
        if (name[1] == '.') {
            out[1] = '.';
        }
        return;
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

/* Поиск записи в каталоге-кластере. Возвращает 1 и заполняет e.
 * Позиция записи (LBA сектора, смещение в секторе) — в lba/off,
 * если указатели не NULL. */
static int dir_find_at(uint16_t cluster, const char *name,
                       struct fat_dirent *e, uint32_t *lba, uint32_t *off)
{
    struct dir_iter it;
    dir_iter_init(&it, cluster);
    while (dir_iter_next(&it, e)) {
        if (name_matches(e, name)) {
            if (lba != NULL) {
                *lba = it.last_lba;
                *off = it.offset - 32;
            }
            return 1;
        }
    }
    return 0;
}

/* Поиск записи в каталоге-кластере (без позиции). */
static int dir_find_in(uint16_t cluster, const char *name,
                       struct fat_dirent *e)
{
    return dir_find_at(cluster, name, e, NULL, NULL);
}

/*
 * Поиск по пути "A/B/FILE.EXT" относительно текущего каталога.
 * Промежуточные компоненты должны быть каталогами. Последний
 * компонент ищется как есть (файл или каталог). Возвращает
 * запись в e (имя последнего компонента), его каталог — в *dir,
 * позицию записи — в lba/off (если указатели не NULL).
 */
static int fs_find_path(const char *path, struct fat_dirent *e,
                        uint16_t *dir, uint32_t *lba, uint32_t *off)
{
    uint16_t cur = (cwd_depth == 0) ? 0 : cwd_stack[cwd_depth - 1];

    if (*path == '\0') {
        return 0;
    }

    char comp[32];
    const char *p = path;
    for (;;) {
        /* при провале любого компонента *dir остаётся каталогом,
         * в котором искался последний компонент (куда создавать) */
        if (dir != NULL) {
            *dir = cur;
        }
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

        if (!dir_find_at(cur, comp, e, last ? lba : NULL,
                         last ? off : NULL)) {
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

    disk_base = (uint8_t *)mod->start;
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
    return fs_find_path(name, e, dir, NULL, NULL);
}

/* Поиск записи с позицией (LBA, смещение). */
static int fs_find_at(const char *name, struct fat_dirent *e, uint16_t *dir,
                      uint32_t *lba, uint32_t *off)
{
    return fs_find_path(name, e, dir, lba, off);
}

/* --- создание и правка записей каталога ------------------------------ */

/* Последний компонент пути (после последнего '/'). */
static const char *path_last_component(const char *path)
{
    const char *last = path;
    for (const char *p = path; *p; p++) {
        if (*p == '/') {
            last = p + 1;
        }
    }
    return last;
}

/* FAT-время/дата из RTC (секунды делятся на 2: точность 2 с). */
static uint16_t fat_time(void)
{
    struct rtc_time t;
    rtc_get_time(&t);
    return (uint16_t)(((uint16_t)t.hour << 11) |
                      ((uint16_t)t.minute << 5) | (t.second / 2));
}

static uint16_t fat_date(void)
{
    struct rtc_time t;
    rtc_get_time(&t);
    uint16_t y = (t.year >= 1980) ? (uint16_t)(t.year - 1980) : 0;
    return (uint16_t)((y << 9) | ((uint16_t)t.month << 5) | t.day);
}

/* Поиск свободной записи (0x00 или 0xE5) в каталоге. Для подкаталога
 * при заполненности выделяет дополнительный кластер. */
static int dir_find_free(uint16_t cluster, uint32_t *lba, uint32_t *off)
{
    uint32_t secs[40];
    uint32_t n = 0;
    uint16_t last = 0;              /* последний кластер подкаталога */
    uint32_t root_start =
        (uint32_t)bpb_reserved + (uint32_t)bpb_fats * bpb_fat_size;

    if (cluster == 0) {
        for (uint32_t s = 0; s < root_dir_sectors; s++) {
            secs[n++] = root_start + s;
        }
    } else {
        uint16_t c = cluster;
        while (c != 0x0FFF && n < sizeof(secs) / sizeof(secs[0])) {
            last = c;
            for (uint32_t s = 0; s < bpb_spc; s++) {
                secs[n++] = cluster_sector(c) + s;
            }
            c = fat_next(c);
        }
        if (n >= sizeof(secs) / sizeof(secs[0])) {
            return 0;
        }
        /* цепочка прошла целиком, свободной записи нет? — проверим ниже */
        secs[n] = 0;                /* маркер: места хватило */
    }

    uint8_t sec[512];
    for (uint32_t i = 0; i < n; i++) {
        if (!disk_read_sector(secs[i], sec)) {
            return 0;
        }
        for (uint32_t o = 0; o < bpb_bps; o += 32) {
            if (sec[o] == 0x00 || sec[o] == 0xE5) {
                *lba = secs[i];
                *off = o;
                return 1;
            }
        }
    }

    /* свободных записей нет: подкаталог расширяем кластером */
    if (cluster == 0) {
        return 0;                   /* корень фиксированного размера */
    }
    uint16_t nc = alloc_cluster();
    if (nc == 0) {
        return 0;
    }
    fat_write_entry(last, nc);
    uint8_t zero[512];
    memset(zero, 0, sizeof(zero));
    for (uint32_t s = 0; s < bpb_spc; s++) {
        if (!disk_write_sector(cluster_sector(nc) + s, zero)) {
            return 0;
        }
    }
    *lba = cluster_sector(nc);
    *off = 0;
    return 1;
}

/* Добавление новой записи в каталог. */
static int dir_create_entry(uint16_t dir, const char *name, uint8_t attr,
                            uint16_t first_cluster, uint32_t size)
{
    uint32_t lba, off;
    if (!dir_find_free(dir, &lba, &off)) {
        return 0;
    }
    struct fat_dirent e;
    memset(&e, 0, sizeof(e));
    name_to_83(name, (char *)e.name);
    e.attr = attr;
    e.crt_time = fat_time();
    e.crt_date = fat_date();
    e.wrt_time = e.crt_time;
    e.wrt_date = e.crt_date;
    e.first_cluster = first_cluster;
    e.file_size = size;

    uint8_t sec[512];
    if (!disk_read_sector(lba, sec)) {
        return 0;
    }
    memcpy(sec + off, &e, sizeof(e));
    return disk_write_sector(lba, sec);
}

/* Обновление записи существующего файла (кластер и размер). */
static int dir_update_entry(uint16_t dir, const char *name,
                            uint16_t first_cluster, uint32_t size)
{
    struct fat_dirent e;
    uint32_t lba, off;
    if (!dir_find_at(dir, name, &e, &lba, &off)) {
        return 0;
    }
    e.first_cluster = first_cluster;
    e.file_size = size;
    e.wrt_time = fat_time();
    e.wrt_date = fat_date();

    uint8_t sec[512];
    if (!disk_read_sector(lba, sec)) {
        return 0;
    }
    memcpy(sec + off, &e, sizeof(e));
    return disk_write_sector(lba, sec);
}

/* Стирание записи каталога: 0xE5 + обнуление (как в x16-PRos). */
static int dir_delete_entry(uint32_t lba, uint32_t off)
{
    uint8_t sec[512];
    if (!disk_read_sector(lba, sec)) {
        return 0;
    }
    sec[off] = 0xE5;
    memset(sec + off + 1, 0, 31);
    return disk_write_sector(lba, sec);
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

/* --- запись: публичное API -------------------------------------------- */

/*
 * Запись файла (создание или полная перезапись), стиль x16-PRos:
 * существующий файл удаляется и создаётся заново.
 * Каталог с таким именем — ошибка. Возвращает 1 при успехе.
 */
int fs_write(const char *name, const void *data, uint32_t size)
{
    if (disk_base == NULL || name == NULL || *name == '\0') {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    if (!fs_find(name, &e, &dir)) {
        /* новый файл: dir — родительский каталог (из fs_find_path) */
        uint16_t first = 0;
        if (size > 0 && !write_chain(&first, data, size)) {
            return 0;
        }
        return dir_create_entry(dir, path_last_component(name),
                                FAT_ATTR_ARCHIVE, first, size);
    }
    if (e.attr & FAT_ATTR_DIR) {
        return 0;                   /* каталог не перезаписываем */
    }
    /* перезапись: освобождаем старую цепочку */
    free_chain(e.first_cluster);
    uint16_t first = 0;
    if (size > 0 && !write_chain(&first, data, size)) {
        return 0;
    }
    return dir_update_entry(dir, path_last_component(name), first, size);
}

/* Удаление файла. Возвращает 1 при успехе. */
int fs_remove(const char *name)
{
    if (disk_base == NULL || name == NULL || *name == '\0') {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    uint32_t lba, off;
    if (!fs_find_at(name, &e, &dir, &lba, &off)) {
        return 0;
    }
    if (e.attr & FAT_ATTR_DIR) {
        return 0;                   /* каталоги — через fs_rmdir */
    }
    free_chain(e.first_cluster);
    return dir_delete_entry(lba, off);
}

/* Переименование файла или каталога. Возвращает 1 при успехе. */
int fs_rename(const char *oldname, const char *newname)
{
    if (disk_base == NULL || oldname == NULL || *oldname == '\0' ||
        newname == NULL || *newname == '\0') {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    uint32_t lba, off;
    if (!fs_find_at(oldname, &e, &dir, &lba, &off)) {
        return 0;
    }
    if (fs_exists(newname)) {
        return 0;                   /* цель уже занята */
    }
    name_to_83(path_last_component(newname), (char *)e.name);

    uint8_t sec[512];
    if (!disk_read_sector(lba, sec)) {
        return 0;
    }
    memcpy(sec + off, &e, sizeof(e));
    return disk_write_sector(lba, sec);
}

/* Является ли имя каталогом (1) или файлом/отсутствующим (0). */
int fs_is_dir(const char *name)
{
    if (disk_base == NULL || name == NULL || *name == '\0') {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    if (!fs_find(name, &e, &dir)) {
        return 0;
    }
    return (e.attr & FAT_ATTR_DIR) ? 1 : 0;
}

/* Создание каталога (с записями "." и ".."). Возвращает 1 при успехе. */
int fs_mkdir(const char *name)
{
    if (disk_base == NULL || name == NULL || *name == '\0') {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    if (fs_find(name, &e, &dir)) {
        return 0;                   /* уже существует */
    }
    uint16_t first = alloc_cluster();
    if (first == 0) {
        return 0;
    }

    /* "." и ".." в первом секторе кластера */
    struct fat_dirent dot;
    memset(&dot, 0, sizeof(dot));
    name_to_83(".", (char *)dot.name);
    dot.attr = FAT_ATTR_DIR;
    dot.first_cluster = first;
    struct fat_dirent dotdot;
    memset(&dotdot, 0, sizeof(dotdot));
    name_to_83("..", (char *)dotdot.name);
    dotdot.attr = FAT_ATTR_DIR;
    dotdot.first_cluster = dir;     /* родитель; 0 = корень */

    uint8_t sec[512];
    memset(sec, 0, sizeof(sec));
    memcpy(sec + 0, &dot, sizeof(dot));
    memcpy(sec + 32, &dotdot, sizeof(dotdot));
    for (uint32_t s = 0; s < bpb_spc; s++) {
        if (!disk_write_sector(cluster_sector(first) + s, sec)) {
            return 0;
        }
        memset(sec, 0, sizeof(sec));
    }
    return dir_create_entry(dir, path_last_component(name),
                            FAT_ATTR_DIR, first, 0);
}

/* Удаление пустого каталога. Возвращает 1 при успехе. */
int fs_rmdir(const char *name)
{
    if (disk_base == NULL || name == NULL || *name == '\0') {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    uint32_t lba, off;
    if (!fs_find_at(name, &e, &dir, &lba, &off)) {
        return 0;
    }
    if (!(e.attr & FAT_ATTR_DIR)) {
        return 0;                   /* это файл */
    }
    /* пустота: допустимы только "." и ".." */
    struct dir_iter it;
    struct fat_dirent ent;
    dir_iter_init(&it, e.first_cluster);
    while (dir_iter_next(&it, &ent)) {
        if (ent.name[0] == '.' &&
            (ent.name[1] == ' ' || ent.name[1] == '.')) {
            continue;
        }
        return 0;                   /* каталог не пуст */
    }
    free_chain(e.first_cluster);
    return dir_delete_entry(lba, off);
}

/* Свободное место на диске, байт (свободные кластеры). */
uint32_t fs_free_space(void)
{
    if (disk_base == NULL) {
        return 0;
    }
    uint32_t free = 0;
    uint16_t maxc = max_cluster();
    for (uint16_t c = 2; c < maxc; c++) {
        if (fat_entry(c) == 0) {
            free++;
        }
    }
    return free * (uint32_t)bpb_bps * bpb_spc;
}

/*
 * Сводка каталога: число записей и суммарный размер файлов (байт),
 * подкаталоги не считаются. Возвращает число записей или 0xFFFFFFFF
 * при ошибке (нет каталога).
 */
uint32_t fs_dir_info(const char *dir, uint32_t *bytes)
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
    uint32_t count = 0, total = 0;
    dir_iter_init(&it, start);
    while (dir_iter_next(&it, &e)) {
        if (e.name[0] == '.' &&
            (e.name[1] == ' ' || e.name[1] == '.')) {
            continue;
        }
        count++;
        if (!(e.attr & FAT_ATTR_DIR)) {
            total += e.file_size;
        }
    }
    if (bytes != NULL) {
        *bytes = total;
    }
    return count;
}

/*
 * Обновление времени файла/каталога: создаёт пустой файл, если его
 * нет (стиль touch), иначе только обновляет запись каталога, не
 * трогая содержимое.
 */
int fs_touch(const char *name)
{
    if (disk_base == NULL || name == NULL || *name == '\0') {
        return 0;
    }
    struct fat_dirent e;
    uint16_t dir;
    if (!fs_find(name, &e, &dir)) {
        return dir_create_entry(dir, path_last_component(name),
                                FAT_ATTR_ARCHIVE, 0, 0);
    }
    return dir_update_entry(dir, path_last_component(name),
                            e.first_cluster, e.file_size);
}
