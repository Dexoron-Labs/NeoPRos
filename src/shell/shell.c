#include "shell/shell.h"
#include "shell/readline.h"
#include "kernel/kbd.h"
#include "kernel/console.h"
#include "lib/string.h"

/* Имя и приглашение оболочки. Чтобы "сменить prosh на sh",
 * достаточно поменять эти строки — остальной код их использует.
 * Приглашение — в формате x16-PRos: "[user@NeoPRos] > ". */
const char *shell_name = "prosh";
const char *shell_prompt = "[user@NeoPRos] > ";

/* Глобальная таблица команд */
static const struct shell_command *commands[SHELL_MAX_COMMANDS];

/* Глобальная таблица команд */
static const struct shell_command *commands[SHELL_MAX_COMMANDS];
static uint32_t command_count;

/* Обработчик неизвестных команд: 0 — обработано, -1 — не распознано. */
static int (*exec_fallback)(int argc, char **argv);

void shell_set_fallback(int (*fn)(int argc, char **argv))
{
    exec_fallback = fn;
}

void shell_register(const struct shell_command *cmd)
{
    if (cmd == NULL || cmd->name == NULL ||
        command_count >= SHELL_MAX_COMMANDS) {
        return;
    }
    /* дубликаты не регистрируем */
    if (shell_find(cmd->name) != NULL) {
        return;
    }
    commands[command_count++] = cmd;
}

const struct shell_command *shell_find(const char *name)
{
    for (uint32_t i = 0; i < command_count; i++) {
        const char *n = commands[i]->name;
        uint32_t j = 0;
        int match = 1;
        while (name[j] && n[j]) {
            if (to_lower(name[j]) != to_lower(n[j])) {
                match = 0;
                break;
            }
            j++;
        }
        if (match && name[j] == '\0' && n[j] == '\0') {
            return commands[i];
        }
    }
    return NULL;
}

const struct shell_command *shell_iter(uint32_t index)
{
    if (index >= command_count) {
        return NULL;
    }
    return commands[index];
}

/*
 * Разбор строки на аргументы. Исходная строка мутирует:
 * слова заменяются на '\0'-терминированные, argv указывают внутрь.
 *
 * Правила:
 *   - разделители слов — пробелы и табы;
 *   - кавычки " и ' группируют слова и удаляются;
 *   - обратный слэш экранирует следующий символ.
 */
static uint32_t shell_tokenize(char *line, char **argv)
{
    uint32_t argc = 0;
    char *src = line;
    char *dst = line;
    char quote = 0;

    for (;;) {
        while (*src == ' ' || *src == '\t') {
            src++;
        }
        if (*src == '\0') {
            break;
        }

        if (argc < SHELL_MAX_ARGS) {
            argv[argc] = dst;
        }
        argc++;

        while (*src) {
            char c = *src++;
            if (quote) {
                if (c == quote) {
                    quote = 0;          /* закрывающая кавычка */
                } else {
                    *dst++ = c;
                }
                continue;
            }
            if (c == '"' || c == '\'') {
                quote = c;              /* открывающая кавычка */
                continue;
            }
            if (c == '\\' && *src) {
                *dst++ = *src++;
                continue;
            }
            if (c == ' ' || c == '\t') {
                break;
            }
            *dst++ = c;
        }
        *dst++ = '\0';
    }
    *dst = '\0';
    return argc;
}

int shell_exec(int argc, char **argv)
{
    if (argc == 0) {
        return 0;
    }

    const struct shell_command *cmd = shell_find(argv[0]);
    if (cmd == NULL) {
        if (exec_fallback != NULL && exec_fallback(argc, argv) == 0) {
            return 0;
        }
        /* сообщение в стиле x16-PRos (invalid_msg) */
        console_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        console_puts("No such command or program\n");
        console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return 0;
    }
    return cmd->handler(argc, argv);
}

int shell_run_line(const char *line)
{
    /* копия на стеке: tokenize мутирует строку, а вызывающий
     * может захотеть сохранить исходный текст */
    char copy[READLINE_MAX_LEN + 1];
    char *argv[SHELL_MAX_ARGS];
    uint32_t argc;

    strcpy(copy, line);
    argc = shell_tokenize(copy, argv);
    if (argc == 0) {
        return 0;
    }
    return shell_exec(argc, argv);
}

/* --- интерфейс при старте (стиль x16-PRos, kernel.asm) ------------ */

/* Верхняя рамка-заголовок: блоки 0xB0/0xB1/0xB2/0xDB с именем ОС. */
static void print_header(void)
{
    int i;

    for (i = 0; i < 16; i++) {
        console_putc(0xB0);
    }
    for (i = 0; i < 8; i++) {
        console_putc(0xB1);
    }
    for (i = 0; i < 6; i++) {
        console_putc(0xB2);
    }
    console_putc(0xDB);
    console_putc(0xDB);
    console_puts(" NeoPRos v0.1.0 ");
    console_putc(0xDB);
    console_putc(0xDB);
    for (i = 0; i < 6; i++) {
        console_putc(0xB2);
    }
    for (i = 0; i < 8; i++) {
        console_putc(0xB1);
    }
    for (i = 0; i < 16; i++) {
        console_putc(0xB0);
    }
    console_putc('\n');
}

/* ASCII-арт "NeoPRos" (тот же стиль, что арт "PRos" в x16-PRos). */
static const char *logo_art[] = {
    "  _   _   _____   ____   _____  ____   _____ ",
    " | \\ | | |  __ \\ / __ \\ / ____|/ __ \\ / ____|",
    " |  \\| | | |__) | |  | | (___ | |  | | (___  ",
    " | . ` | |  ___/| |  | |\\___ \\| |  | |\\___ \\ ",
    " | |\\  | | |    | |__| |____) | |__| |____) |",
    " |_| \\_| |_|    |\\____/|_____/ \\____/|_____/ ",
};

/* Полоса из 15 цветовых блоков (цвета 0-14), как в x16-PRos. */
static void print_color_blocks(void)
{
    for (int i = 0; i < 15; i++) {
        console_set_color((uint8_t)i, VGA_COLOR_BLACK);
        console_putc(0xDB);
    }
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    console_putc('\n');
}

int shell_main(void)
{
    console_clear();

    print_header();
    console_puts("\n\n");

    for (uint32_t i = 0; i < sizeof(logo_art) / sizeof(logo_art[0]); i++) {
        console_puts(logo_art[i]);
        console_puts("\n");
    }
    console_puts("\n");

    console_puts("* Copyright (C) 2026 NeoPRos Project\n");
    console_puts("* Shell: NeoPRos Terminal v0.1.0\n\n");

    console_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    console_puts("Type HELP to get list of the commands\n");
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    print_color_blocks();
    console_puts("\n\n");

    for (;;) {
        char *line = readline(shell_prompt);
        int result = shell_run_line(line);
        if (result == SHELL_EXIT) {
            return SHELL_EXIT;
        }
    }
}
