#include "shell/shell.h"
#include "shell/readline.h"
#include "kernel/console.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/kheap.h"
#include "kernel/pit.h"
#include "kernel/rtc.h"
#include "kernel/io.h"
#include "fs/fs.h"
#include "loader/loader.h"

/* Код завершения оболочки (переопределяем, чтобы не тянуть shell.h) */
#define CMD_EXIT SHELL_EXIT

/* Символ линкера: конец памяти ядра */
extern char _mb_bss_end[];

/* --- команды ----------------------------------------------------- */

/* Вспомогательные функции в стиле x16-PRos (print_string_*) */
static void print_green(const char *s)
{
    console_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    console_puts(s);
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}
static void print_cyan(const char *s)
{
    console_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    console_puts(s);
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}
static void print_red(const char *s)
{
    console_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    console_puts(s);
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

static int cmd_help(int argc, char **argv)
{
    const struct shell_command *cmd;

    if (argc > 1) {
        cmd = shell_find(argv[1]);
        if (cmd == NULL) {
            print_red("help: no such command: ");
            console_puts(argv[1]);
            console_putc('\n');
            return 0;
        }
        console_puts(cmd->name);
        console_puts(" - ");
        console_puts(cmd->help);
        console_putc('\n');
        return 0;
    }

    /* формат как в x16-PRos: "CMD  <arg>  - description", верхний регистр */
    for (uint32_t i = 0; (cmd = shell_iter(i)) != NULL; i++) {
        const char *n = cmd->name;
        uint32_t len = 0;
        while (n[len]) {
            console_putc((char)to_upper(n[len]));
            len++;
        }
        while (len < 20) {
            console_putc(' ');
            len++;
        }
        console_puts("- ");
        console_puts(cmd->help);
        console_putc('\n');
    }
    return 0;
}

static int cmd_ver(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    console_puts("\n");
    console_puts("NeoPRos Terminal v0.1.0\n");  /* как version_msg в x16 */
    return 0;
}

static int cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    uint32_t total_kb = pmm_total_page_count() * PMM_PAGE_SIZE / 1024;
    uint32_t free_kb = pmm_free_page_count() * PMM_PAGE_SIZE / 1024;

    console_puts("\n");
    console_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    for (int i = 0; i < 20; i++) {
        console_putc(0xC4);
    }
    console_puts(" INFO ");
    for (int i = 0; i < 21; i++) {
        console_putc(0xC4);
    }
    console_puts("\n");
    console_puts("  NeoPRos is the simple 32 bit operating\n");
    console_puts("  system written in C for i386 PC`s\n");
    for (int i = 0; i < 47; i++) {
        console_putc(0xC4);
    }
    console_puts("\n");
    console_puts("  Author:           AI\n");
    console_puts("  Source code:      GitHub (https://github.com/anomalyco/NeoPRos)\n");
    console_puts("  License:          GPL-3.0-or-later\n");
    console_puts("  OS version:       0.1.0\n");
    kprintf("  Memory:           %u KiB total, %u KiB free\n",
            total_kb, free_kb);
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    return 0;
}

static int cmd_cls(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    console_clear();
    return 0;
}

static int cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            console_putc(' ');
        }
        console_puts(argv[i]);
    }
    console_putc('\n');
    return 0;
}

static int cmd_date(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    struct rtc_time t;
    rtc_get_time(&t);
    /* формат x16-PRos: DD/MM/YY, значение голубым */
    console_puts("Current date: ");
    char buf[16];
    ksnprintf(buf, sizeof(buf), "%02u/%02u/%02u\n",
              t.day, t.month, t.year % 100);
    print_cyan(buf);
    return 0;
}

static int cmd_time(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    struct rtc_time t;
    rtc_get_time(&t);
    console_puts("Current time: ");
    char buf[16];
    ksnprintf(buf, sizeof(buf), "%02u:%02u:%02u\n",
              t.hour, t.minute, t.second);
    print_cyan(buf);
    return 0;
}

static int cmd_memory(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    uint32_t total_kb = pmm_total_page_count() * PMM_PAGE_SIZE / 1024;
    uint32_t free_kb = pmm_free_page_count() * PMM_PAGE_SIZE / 1024;
    uint32_t kernel_kb = ((uint32_t)_mb_bss_end - 0x100000u) / 1024;

    kprintf("Physical memory:\n");
    kprintf("  total: %u KiB\n", total_kb);
    kprintf("  free:  %u KiB\n", free_kb);
    kprintf("  used:  %u KiB (kernel: %u KiB)\n",
            total_kb - free_kb, kernel_kb);
    kprintf("Kernel heap:\n");
    kprintf("  total: %u KiB\n", kheap_total() / 1024);
    kprintf("  used:  %u KiB\n", kheap_used() / 1024);
    return 0;
}

static int cmd_uptime(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    uint32_t secs = pit_ticks() / PIT_HZ;
    kprintf("Uptime: %u s (%u:%02u:%02u)\n", secs,
            secs / 3600, (secs / 60) % 60, secs % 60);
    return 0;
}

static int cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("Rebooting...\n");
    outb(0x64, 0xFE);       /* системный сброс 8042 (работает в QEMU) */
    return 0;
}

static int cmd_shutdown(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("Shutting down...\n");
    outw(0x604, 0x2000);    /* ACPI-порт выключения QEMU */
    return 0;
}

static int cmd_exit(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return CMD_EXIT;
}

/* --- калькулятор: рекурсивный спуск ------------------------------ */

/*
 * Грамматика (целочисленная арифметика):
 *   expr    := term (('+' | '-') term)*
 *   term    := factor (('*' | '/' | '%') factor)*
 *   factor  := '-' factor | '(' expr ')' | number
 *   number  := 0x<hex> | <digits>
 */

struct calc_parser {
    const char *src;
    const char *pos;
    int error;
};

static void calc_skip_space(struct calc_parser *p)
{
    while (*p->pos == ' ' || *p->pos == '\t') {
        p->pos++;
    }
}

static int calc_number(struct calc_parser *p)
{
    calc_skip_space(p);

    uint32_t value = 0;
    int digits = 0;

    if (p->pos[0] == '0' && (p->pos[1] == 'x' || p->pos[1] == 'X')) {
        p->pos += 2;
        while (1) {
            char c = *p->pos;
            uint32_t d;
            if (c >= '0' && c <= '9') {
                d = (uint32_t)(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                d = (uint32_t)(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                d = (uint32_t)(c - 'A' + 10);
            } else {
                break;
            }
            value = value * 16 + d;
            digits++;
            p->pos++;
        }
    } else {
        while (*p->pos >= '0' && *p->pos <= '9') {
            value = value * 10 + (uint32_t)(*p->pos - '0');
            digits++;
            p->pos++;
        }
    }

    if (digits == 0) {
        p->error = 1;
        return 0;
    }
    return (int)value;
}

static int calc_factor(struct calc_parser *p);

static int calc_term(struct calc_parser *p);

static int calc_expr(struct calc_parser *p)
{
    int result = calc_term(p);

    while (!p->error) {
        calc_skip_space(p);
        char op = *p->pos;
        if (op != '+' && op != '-') {
            break;
        }
        p->pos++;
        int rhs = calc_term(p);
        result = (op == '+') ? result + rhs : result - rhs;
    }
    return result;
}

static int calc_term(struct calc_parser *p)
{
    int result = calc_factor(p);

    while (!p->error) {
        calc_skip_space(p);
        char op = *p->pos;
        if (op != '*' && op != '/' && op != '%') {
            break;
        }
        p->pos++;
        int rhs = calc_factor(p);
        if (op == '*') {
            result *= rhs;
        } else if (rhs == 0) {
            p->error = 1;       /* деление на ноль */
            return 0;
        } else if (op == '/') {
            result /= rhs;
        } else {
            result %= rhs;
        }
    }
    return result;
}

static int calc_factor(struct calc_parser *p)
{
    calc_skip_space(p);

    if (*p->pos == '-') {
        p->pos++;
        return -calc_factor(p);
    }
    if (*p->pos == '+') {
        p->pos++;
        return calc_factor(p);
    }
    if (*p->pos == '(') {
        p->pos++;
        int result = calc_expr(p);
        calc_skip_space(p);
        if (*p->pos != ')') {
            p->error = 1;
            return 0;
        }
        p->pos++;
        return result;
    }
    return calc_number(p);
}

static int cmd_calc(int argc, char **argv)
{
    /* склеиваем аргументы в одну строку выражения */
    char expr[256];
    char *d = expr;
    for (int i = 1; i < argc && (uint32_t)(d - expr) < sizeof(expr) - 1; i++) {
        const char *s = argv[i];
        if (i > 1) {
            *d++ = ' ';
        }
        while (*s && (uint32_t)(d - expr) < sizeof(expr) - 1) {
            *d++ = *s++;
        }
    }
    *d = '\0';

    struct calc_parser p = { expr, expr, 0 };
    int result = calc_expr(&p);

    calc_skip_space(&p);
    if (p.error || *p.pos != '\0') {
        kprintf("calc: syntax error\n");
        return 0;
    }
    kprintf("%d\n", result);
    return 0;
}

/* --- файловая система ------------------------------------------------ */

/* Дописывает .BIN, если в имени нет расширения (как в x16-PRos). */
static void expand_bin_name(const char *name, char *out, uint32_t size)
{
    const char *dot = NULL;
    for (const char *p = name; *p; p++) {
        if (*p == '.') {
            dot = p;
        }
    }
    if (dot != NULL) {
        strcpy(out, name);
        return;
    }
    uint32_t n = strlen(name);
    if (n + 5 >= size) {
        out[0] = '\0';
        return;
    }
    strcpy(out, name);
    strcpy(out + n, ".BIN");
}

static int cmd_pwd(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char cwd[128];
    fs_getcwd(cwd, sizeof(cwd));
    kprintf("%s\n", cwd);
    return 0;
}

static int cmd_cd(int argc, char **argv)
{
    char cwd[128];

    if (argc < 2) {
        /* как в x16-PRos: показываем текущий каталог */
        fs_getcwd(cwd, sizeof(cwd));
        console_puts("Current directory: ");
        console_puts(strcmp(cwd, "/") == 0 ? "A:/" : cwd + 1);
        console_putc('\n');
        return 0;
    }
    if (!fs_chdir(argv[1])) {
        print_red("Directory not found or invalid\n");
        return 0;
    }
    print_green("Directory changed\n");
    fs_getcwd(cwd, sizeof(cwd));
    console_puts("Current directory: ");
    console_puts(strcmp(cwd, "/") == 0 ? "A:/" : cwd + 1);
    console_putc('\n');
    return 0;
}

static int cmd_cat(int argc, char **argv)
{
    if (argc < 2) {
        print_red("No filename or not enough filenames\n");
        return 0;
    }
    uint32_t size = fs_size(argv[1]);
    if (size == 0 && !fs_exists(argv[1])) {
        print_red("File not found\n");
        return 0;
    }
    static char buf[4096];
    uint32_t got = fs_load(argv[1], buf, sizeof(buf));
    if (got == 0) {
        return 0;
    }
    /* печатаем только печатный ASCII (бинарные файлы не портят экран) */
    char last = '\n';
    for (uint32_t i = 0; i < got; i++) {
        char c = buf[i];
        if (c == '\n' || c == '\t' || (c >= 0x20 && c <= 0x7E)) {
            console_putc(c);
            last = c;
        } else {
            console_putc('.');
            last = '.';
        }
    }
    if (last != '\n') {
        console_putc('\n');
    }
    if (got == sizeof(buf) && size > got) {
        print_red("File is too big to display\n");
    }
    return 0;
}

static int cmd_size(int argc, char **argv)
{
    if (argc < 2) {
        print_red("No filename or not enough filenames\n");
        return 0;
    }
    uint32_t s = fs_size(argv[1]);
    if (s == 0 && !fs_exists(argv[1])) {
        print_red("File not found\n");
        return 0;
    }
    char num[16];
    ksnprintf(num, sizeof(num), "%u bytes\n", s);
    print_green(num);
    return 0;
}

static int cmd_fs(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (!fs_available()) {
        kprintf("FS: no RAM disk (run with -initrd)\n");
        return 0;
    }
    kprintf("FS: FAT12 ramdisk, %u bytes, module: %s\n",
            fs_image_size(), fs_image_name());
    return 0;
}

/* dir — листинг с размерами и итогом (стиль x16-PRos). */
/* --- листинг в стиле x16-PRos (list_directory, kernel.asm) -------- */

/*
 * Формат строки из fs_list: "NAME.EXT SIZE\n", каталог: "NAME/ SIZE\n".
 * Поле имени 12 символов: "NAME     EXT" (8 + пробел + 3), как в x16.
 */
static void print_list_line(char *line)
{
    /* имя заканчивается на первом пробеле (буфер не мутируем!) */
    char *sp = NULL;
    for (char *c = line; *c; c++) {
        if (*c == ' ') {
            sp = c;
            break;
        }
    }
    if (sp == NULL) {
        return;
    }
    char *size_str = sp + 1;

    int is_dir = 0;
    uint32_t nlen = (uint32_t)(sp - line);
    if (nlen > 0 && line[nlen - 1] == '/') {
        is_dir = 1;
        nlen--;
    }

    char *dot = NULL;
    for (uint32_t i = 0; i < nlen; i++) {
        if (line[i] == '.') {
            dot = &line[i];
        }
    }
    uint32_t base_len = (dot != NULL) ? (uint32_t)(dot - line) : nlen;
    uint32_t ext_len = (dot != NULL) ? nlen - (uint32_t)(dot - line) - 1 : 0;

    /* поле имени: 8 + пробел + 3 (без расширения: 8 + 4 пробела) */
    for (uint32_t i = 0; i < 8; i++) {
        console_putc(i < base_len ? line[i] : ' ');
    }
    if (ext_len > 0) {
        console_putc(' ');
        for (uint32_t i = 0; i < 3; i++) {
            console_putc(i < ext_len ? dot[1 + i] : ' ');
        }
    } else {
        for (int i = 0; i < 4; i++) {
            console_putc(' ');
        }
    }
    console_puts("  ");

    /* размер (5 цифр справа) или метка каталога */
    uint32_t sz = 0;
    for (char *c = size_str; *c >= '0' && *c <= '9'; c++) {
        sz = sz * 10 + (uint32_t)(*c - '0');
    }
    if (is_dir) {
        console_set_color(VGA_COLOR_MAGENTA, VGA_COLOR_BLACK);
        console_puts("<DIR>");
        console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        for (int i = 0; i < 7; i++) {
            console_putc(' ');
        }
    } else {
        uint32_t digits = 1;
        uint32_t t = sz;
        while (t >= 10) {
            t /= 10;
            digits++;
        }
        for (uint32_t i = digits; i < 5; i++) {
            console_putc(' ');
        }
        console_print_dec32(sz);
        for (int i = 0; i < 7; i++) {
            console_putc(' ');
        }
    }
}

static int cmd_dir(int argc, char **argv)
{
    static char buf[2048];
    const char *dir = (argc > 1) ? argv[1] : NULL;
    uint32_t count = fs_list(dir, buf, sizeof(buf));

    if (count == 0xFFFFFFFFu) {
        print_red("File not found\n");
        return 0;
    }

    /* заголовок "A:/" + текущий каталог */
    char cwd[128];
    fs_getcwd(cwd, sizeof(cwd));
    console_puts("\n");
    console_puts("A:/");
    if (strcmp(cwd, "/") != 0) {
        console_puts(cwd + 1);
    }
    console_puts("\n\n");

    /* строки по 3 записи в колонке 26 символов */
    uint32_t col = 0;
    char *line = buf;
    while (*line) {
        print_list_line(line);
        col++;
        if (col == 3) {
            console_putc('\n');
            col = 0;
        }
        while (*line && *line != '\n') {
            line++;
        }
        if (*line == '\n') {
            line++;
        }
    }
    if (col != 0) {
        console_putc('\n');
    }

    /* итог: "N files", занято и свободно (как в x16-PRos) */
    uint32_t total_kb = fs_image_size() / 1024;
    uint32_t free_kb = fs_free_space() / 1024;
    uint32_t used_kb = total_kb - free_kb;
    char num[16];

    console_puts("\n");
    ksnprintf(num, sizeof(num), "%u", count);
    print_cyan(num);
    console_puts(" files   ");
    ksnprintf(num, sizeof(num), "%u", used_kb);
    print_green(num);
    console_puts(" KB\n");
    ksnprintf(num, sizeof(num), "%u", free_kb);
    print_green(num);
    console_puts(" KB free\n\n");
    return 0;
}

/* del — удаление файла (сообщения как в x16-PRos). */
static int cmd_del(int argc, char **argv)
{
    if (argc < 2) {
        print_red("No filename or not enough filenames\n");
        return 0;
    }
    if (!fs_remove(argv[1])) {
        print_red("File not found\n");
        return 0;
    }
    print_green("Deleted file.\n");
    return 0;
}

/* copy — копирование файла (до 32 КБ, как в x16-PRos). */
static int cmd_copy(int argc, char **argv)
{
    if (argc < 3) {
        print_red("No filename or not enough filenames\n");
        return 0;
    }
    if (fs_is_dir(argv[1])) {
        print_red("File not found\n");
        return 0;
    }
    if (fs_exists(argv[2])) {
        print_red("Target file already exists!\n");
        return 0;
    }
    static char buf[32768];
    uint32_t size = fs_load(argv[1], buf, sizeof(buf));
    if (size == 0) {
        if (fs_exists(argv[1])) {
            print_red("File is too big\n");
        } else {
            print_red("File not found\n");
        }
        return 0;
    }
    if (!fs_write(argv[2], buf, size)) {
        print_red("Could not write file. Write protected or invalid filename?\n");
        return 0;
    }
    print_green("File copied successfully\n");
    return 0;
}

/* ren — переименование файла или каталога. */
static int cmd_ren(int argc, char **argv)
{
    if (argc < 3) {
        print_red("No filename or not enough filenames\n");
        return 0;
    }
    if (!fs_rename(argv[1], argv[2])) {
        print_red("File not found\n");
        return 0;
    }
    print_green("File renamed successfully\n");
    return 0;
}

/* touch — создать пустой файл. */
static int cmd_touch(int argc, char **argv)
{
    if (argc < 2) {
        print_red("No filename or not enough filenames\n");
        return 0;
    }
    if (!fs_touch(argv[1])) {
        print_red("Could not create file\n");
        return 0;
    }
    print_green("File created successfully\n");
    return 0;
}

/* write — создать текстовый файл с заданным содержимым. */
static int cmd_write(int argc, char **argv)
{
    if (argc < 3) {
        print_red("No filename or not enough filenames\n");
        return 0;
    }
    if (!fs_write(argv[1], argv[2], strlen(argv[2]))) {
        print_red("Could not write file. Write protected or invalid filename?\n");
        return 0;
    }
    print_green("File created successfully\n");
    return 0;
}

/* mkdir — создать каталог. */
static int cmd_mkdir(int argc, char **argv)
{
    if (argc < 2) {
        print_red("No filename or not enough filenames\n");
        return 0;
    }
    if (!fs_mkdir(argv[1])) {
        print_red("Could not create directory\n");
        return 0;
    }
    print_green("Directory created successfully\n");
    return 0;
}

/* deldir — удалить пустой каталог. */
static int cmd_deldir(int argc, char **argv)
{
    if (argc < 2) {
        print_red("No filename or not enough filenames\n");
        return 0;
    }
    if (!fs_rmdir(argv[1])) {
        print_red("Directory not empty or not found\n");
        return 0;
    }
    print_green("Directory deleted successfully\n");
    return 0;
}

/* --- запуск программ --------------------------------------------------- */

/*
 * Запуск .BIN по имени (стиль x16-PRos: программа вводится как команда,
 * расширение .BIN дополняется автоматически). Аргументы — с argv[1].
 */
static int run_program(const char *name, int argc, char **argv)
{
    char fname[16];
    expand_bin_name(name, fname, sizeof(fname));
    if (!fs_exists(fname)) {
        return -1;
    }

    /* командная строка: склеиваем аргументы с 1-го */
    char args[READLINE_MAX_LEN + 1];
    char *d = args;
    for (int i = 1; i < argc; i++) {
        const char *s = argv[i];
        if (i > 1) {
            *d++ = ' ';
        }
        while (*s) {
            *d++ = *s++;
        }
    }
    *d = '\0';

    int code = loader_run(fname, args);
    if (code < 0) {
        kprintf("%s: failed to load\n", fname);
    }
    return 0;
}

/* Неизвестная команда: пробуем исполнить как .BIN-файл. */
static int exec_as_program(int argc, char **argv)
{
    return run_program(argv[0], argc, argv);
}

/* --- таблица команд ---------------------------------------------- */

static const struct shell_command shell_commands[] = {
    { "help",     "show this help",                  cmd_help },
    { "ver",      "print OS version",                cmd_ver },
    { "info",     "print system information",        cmd_info },
    { "cls",      "clear the screen",                cmd_cls },
    { "clear",    "alias for cls",                   cmd_cls },
    { "echo",     "print arguments",                 cmd_echo },
    { "date",     "print current date (RTC)",        cmd_date },
    { "time",     "print current time (RTC)",        cmd_time },
    { "calc",     "evaluate arithmetic expression",  cmd_calc },
    { "memory",   "print memory usage",              cmd_memory },
    { "uptime",   "print system uptime",             cmd_uptime },
    { "reboot",   "reboot the system",               cmd_reboot },
    { "shutdown", "power off the system",            cmd_shutdown },
    { "exit",     "exit the shell",                  cmd_exit },
    { "ls",       "list directory (FAT12 ramdisk)",  cmd_dir },
    { "pwd",      "print current directory",         cmd_pwd },
    { "cd",       "change current directory",        cmd_cd },
    { "cat",      "print file contents",             cmd_cat },
    { "size",     "print file size",                 cmd_size },
    { "dir",      "list files with sizes and total", cmd_dir },
    { "del",      "delete a file",                   cmd_del },
    { "copy",     "copy a file",                     cmd_copy },
    { "ren",      "rename a file or directory",      cmd_ren },
    { "touch",    "create empty file or touch time", cmd_touch },
    { "write",    "create text file with data",      cmd_write },
    { "mkdir",    "create a directory",              cmd_mkdir },
    { "deldir",   "delete an empty directory",       cmd_deldir },
    { "fs",       "print filesystem status",         cmd_fs },
};

void commands_init(void)
{
    for (uint32_t i = 0;
         i < sizeof(shell_commands) / sizeof(shell_commands[0]); i++) {
        shell_register(&shell_commands[i]);
    }
    /* неизвестные команды исполняются как .BIN-программы (как в x16-PRos) */
    shell_set_fallback(exec_as_program);
}
