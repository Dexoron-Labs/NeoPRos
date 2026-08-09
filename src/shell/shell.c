#include "shell/shell.h"
#include "shell/readline.h"
#include "kernel/kbd.h"
#include "kernel/console.h"
#include "lib/string.h"

/* Имя и приглашение оболочки. Чтобы "сменить prosh на sh",
 * достаточно поменять эти строки — остальной код их использует. */
const char *shell_name = "prosh";
const char *shell_prompt = "prosh> ";

/* Глобальная таблица команд */
static const struct shell_command *commands[SHELL_MAX_COMMANDS];
static uint32_t command_count;

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
        kprintf("%s: command not found: %s\n", shell_name, argv[0]);
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

int shell_main(void)
{
    console_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    kprintf("NeoPRos shell (%s) - type 'help' for commands.\n", shell_name);
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    for (;;) {
        char *line = readline(shell_prompt);
        int result = shell_run_line(line);
        if (result == SHELL_EXIT) {
            return SHELL_EXIT;
        }
    }
}
