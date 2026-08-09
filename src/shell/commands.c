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

static int cmd_help(int argc, char **argv)
{
    const struct shell_command *cmd;

    if (argc > 1) {
        cmd = shell_find(argv[1]);
        if (cmd == NULL) {
            kprintf("help: no such command: %s\n", argv[1]);
            return 0;
        }
        kprintf("%s - %s\n", cmd->name, cmd->help);
        return 0;
    }

    kprintf("Available commands:\n");
    for (uint32_t i = 0; (cmd = shell_iter(i)) != NULL; i++) {
        kprintf("  %s", cmd->name);
        uint32_t len = strlen(cmd->name);
        while (len < 12) {
            console_putc(' ');
            len++;
        }
        kprintf("%s\n", cmd->help);
    }
    return 0;
}

static int cmd_ver(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    kprintf("NeoPRos 0.1.0 - 32-bit i386 OS, written by AI\n");
    return 0;
}

static int cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    uint32_t total_kb = pmm_total_page_count() * PMM_PAGE_SIZE / 1024;
    uint32_t free_kb = pmm_free_page_count() * PMM_PAGE_SIZE / 1024;
    uint32_t kernel_kb = ((uint32_t)_mb_bss_end - 0x100000u) / 1024;

    kprintf("System info:\n");
    kprintf("  OS:         NeoPRos 0.1.0 (i386)\n");
    kprintf("  Memory:     %u KiB total, %u KiB free\n", total_kb, free_kb);
    kprintf("  Kernel:     %u KiB [0x100000 - 0x%x]\n", kernel_kb,
            (uint32_t)_mb_bss_end);
    kprintf("  Heap:       %u KiB total, %u KiB used\n",
            kheap_total() / 1024, kheap_used() / 1024);
    kprintf("  Uptime:     %u s\n", pit_ticks() / PIT_HZ);
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
    kprintf("%04u-%02u-%02u\n", t.year, t.month, t.day);
    return 0;
}

static int cmd_time(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    struct rtc_time t;
    rtc_get_time(&t);
    kprintf("%02u:%02u:%02u\n", t.hour, t.minute, t.second);
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

static int cmd_ls(int argc, char **argv)
{
    char buf[2048];
    uint32_t count = fs_list((argc > 1) ? argv[1] : NULL, buf, sizeof(buf));

    if (count == 0xFFFFFFFFu) {
        kprintf("ls: %s: no such directory\n",
                (argc > 1) ? argv[1] : "/");
        return 0;
    }
    if (argc > 1) {
        kprintf("%s:\n", argv[1]);
    }
    console_puts(buf);
    kprintf("%u entries\n", count);
    return 0;
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
    if (argc < 2) {
        kprintf("cd: usage: cd <dir>\n");
        return 0;
    }
    if (!fs_chdir(argv[1])) {
        kprintf("cd: %s: no such directory\n", argv[1]);
        return 0;
    }
    return 0;
}

static int cmd_cat(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("cat: usage: cat <file>\n");
        return 0;
    }
    uint32_t size = fs_size(argv[1]);
    if (size == 0 && !fs_exists(argv[1])) {
        kprintf("cat: %s: no such file\n", argv[1]);
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
        kprintf("cat: %s: file truncated (too big)\n", argv[1]);
    }
    return 0;
}

static int cmd_size(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("size: usage: size <file>\n");
        return 0;
    }
    uint32_t s = fs_size(argv[1]);
    if (s == 0 && !fs_exists(argv[1])) {
        kprintf("size: %s: no such file\n", argv[1]);
        return 0;
    }
    kprintf("%s: %u bytes\n", argv[1], s);
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

/* --- запуск программ --------------------------------------------------- */

static int cmd_run(int argc, char **argv)
{
    if (argc < 2) {
        kprintf("run: usage: run <program.bin> [args...]\n");
        return 0;
    }
    char name[16];
    expand_bin_name(argv[1], name, sizeof(name));
    if (!fs_exists(name)) {
        kprintf("run: %s: program not found\n", name);
        return 0;
    }

    /* командная строка: склеиваем аргументы с 3-го */
    char args[READLINE_MAX_LEN + 1];
    char *d = args;
    for (int i = 2; i < argc; i++) {
        const char *s = argv[i];
        if (i > 2) {
            *d++ = ' ';
        }
        while (*s) {
            *d++ = *s++;
        }
    }
    *d = '\0';

    int code = loader_run(name, args);
    if (code < 0) {
        kprintf("run: %s: failed to load\n", name);
        return 0;
    }
    return 0;
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
    { "ls",       "list directory (FAT12 ramdisk)",  cmd_ls },
    { "pwd",      "print current directory",         cmd_pwd },
    { "cd",       "change current directory",        cmd_cd },
    { "cat",      "print file contents",             cmd_cat },
    { "size",     "print file size",                 cmd_size },
    { "fs",       "print filesystem status",         cmd_fs },
    { "run",      "run a .BIN program",              cmd_run },
    { "exec",     "alias for run",                   cmd_run },
};

void commands_init(void)
{
    for (uint32_t i = 0;
         i < sizeof(shell_commands) / sizeof(shell_commands[0]); i++) {
        shell_register(&shell_commands[i]);
    }
}
