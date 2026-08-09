#include "api/sysapi.h"
#include "kernel/console.h"
#include "kernel/kbd.h"
#include "shell/readline.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "kernel/pit.h"
#include "kernel/rtc.h"
#include "fs/fs.h"
#include "loader/loader.h"

/*
 * Реализация системного API (таблицы функций для программ .BIN).
 * Все функции — тонкие обёртки над подсистемами ядра; таблица лежит
 * в BSS ядра, её адрес публикуется по NEOPROS_API_PTR_ADDR (если туда
 * не легла структура multiboot_info — иначе только аргумент _start).
 */

/* --- вывод ---------------------------------------------------------- */

static void api_puts(const char *str)
{
    console_puts(str);
}

static void api_putc(char c)
{
    console_putc(c);
}

static void api_printf(const char *fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    vkprintf(fmt, ap);
    __builtin_va_end(ap);
}

static void api_cls(void)
{
    console_clear();
}

static void api_set_color(uint8_t fg, uint8_t bg)
{
    console_set_color(fg, bg);
}

/* --- ввод ----------------------------------------------------------- */

static int api_getchar(void)
{
    return kbd_getc();
}

static void api_readline(const char *prompt, char *buf, uint32_t size)
{
    char *line = readline(prompt);
    if (size > 0) {
        uint32_t n = strlen(line);
        if (n >= size) {
            n = size - 1;
        }
        memcpy(buf, line, n);
        buf[n] = '\0';
    }
}

/* --- системная информация ------------------------------------------- */

static void api_sysinfo(struct neopros_sysinfo *info)
{
    info->version = NEOPROS_API_VERSION;
    info->mem_total = pmm_total_page_count() * PMM_PAGE_SIZE / 1024;
    info->mem_free = pmm_free_page_count() * PMM_PAGE_SIZE / 1024;
    info->uptime = pit_ticks() / PIT_HZ;
}

static void api_rtc(struct neopros_time *t)
{
    struct rtc_time r;
    rtc_get_time(&r);
    t->year = r.year;
    t->month = r.month;
    t->day = r.day;
    t->hour = r.hour;
    t->minute = r.minute;
    t->second = r.second;
}

/* --- файлы ---------------------------------------------------------- */

static uint32_t api_fs_load(const char *name, void *buf, uint32_t size)
{
    return fs_load(name, buf, size);
}

static uint32_t api_fs_list(const char *dir, char *buf, uint32_t size)
{
    return fs_list(dir, buf, size);
}

static int api_fs_exists(const char *name)
{
    return fs_exists(name) ? 1 : 0;
}

static uint32_t api_fs_size(const char *name)
{
    uint32_t s = fs_size(name);
    return (s == 0) ? 0xFFFFFFFFu : s;
}

static int api_fs_chdir(const char *dir)
{
    return fs_chdir(dir) ? 0 : -1;
}

static void api_fs_getcwd(char *buf, uint32_t size)
{
    fs_getcwd(buf, size);
}

static int api_fs_write(const char *name, const void *data, uint32_t size)
{
    return fs_write(name, data, size);
}

static int api_fs_remove(const char *name)
{
    return fs_remove(name);
}

static int api_fs_rename(const char *oldname, const char *newname)
{
    return fs_rename(oldname, newname);
}

static int api_fs_is_dir(const char *name)
{
    return fs_is_dir(name);
}

static int api_fs_mkdir(const char *name)
{
    return fs_mkdir(name);
}

static int api_fs_rmdir(const char *name)
{
    return fs_rmdir(name);
}

static uint32_t api_fs_free_space(void)
{
    return fs_free_space();
}

/* --- запуск программ ------------------------------------------------ */

static int api_exec(const char *name, const char *args)
{
    return loader_run(name, args);
}

static void api_exit(int code)
{
    loader_exit(code);
}

/* --- таблица -------------------------------------------------------- */

static struct neopros_api sysapi_table = {
    .magic = NEOPROS_API_MAGIC,
    .version = NEOPROS_API_VERSION,

    .puts = api_puts,
    .putc = api_putc,
    .printf = api_printf,
    .cls = api_cls,
    .set_color = api_set_color,

    .getchar = api_getchar,
    .readline = api_readline,

    .fs_load = api_fs_load,
    .fs_list = api_fs_list,
    .fs_exists = api_fs_exists,
    .fs_size = api_fs_size,
    .fs_chdir = api_fs_chdir,
    .fs_getcwd = api_fs_getcwd,

    .sysinfo = api_sysinfo,
    .rtc = api_rtc,

    .exec = api_exec,
    .exit = api_exit,

    .fs_write = api_fs_write,
    .fs_remove = api_fs_remove,
    .fs_rename = api_fs_rename,
    .fs_is_dir = api_fs_is_dir,
    .fs_mkdir = api_fs_mkdir,
    .fs_rmdir = api_fs_rmdir,
    .fs_free_space = api_fs_free_space,
};

void sysapi_init(uint32_t mbi_addr)
{
    /* Публикуем адрес таблицы по фиксированному адресу.
     * Загрузчик (QEMU) кладёт multiboot_info в начало памяти —
     * проверяем, что не перекрываем её. */
    if (mbi_addr == 0 || mbi_addr >= NEOPROS_API_PTR_ADDR + 4) {
        *(struct neopros_api **)NEOPROS_API_PTR_ADDR = &sysapi_table;
    }
}

struct neopros_api *sysapi_get(void)
{
    return &sysapi_table;
}
