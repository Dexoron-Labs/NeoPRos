#include "shell/readline.h"
#include "kernel/kbd.h"
#include "kernel/console.h"
#include "lib/string.h"

/* История: строки 0..count-1, самая свежая в конце. */
static char hist_lines[READLINE_HISTORY_SIZE][READLINE_MAX_LEN + 1];
static uint32_t hist_count;

/* Текущая редактируемая строка */
static char line_buf[READLINE_MAX_LEN + 1];
static uint32_t line_len;
static uint32_t line_pos;          /* позиция курсора в строке */

/* Сохранённая строка на время навигации по истории */
static char saved_line[READLINE_MAX_LEN + 1];
static int saved_line_valid;

/* Курсор навигации по истории: hist_count = строка "сейчас" */
static uint32_t hist_cursor;

void readline_init(void)
{
    hist_count = 0;
    saved_line_valid = 0;
    memset(hist_lines, 0, sizeof(hist_lines));
}

/* --- внутренняя перерисовка строки ------------------------------ */

/* Текущая "напечатанная" длина строки (для затирания хвоста) */
static uint32_t printed_len;

/* Приглашение, показанное при последней перерисовке */
static const char *active_prompt;

/*
 * Перерисовка строки ввода.
 * Порядок: \r → пробелы поверх ВСЕЙ старой печати (приглашение +
 * строка) → \r → заново приглашение + строка. Только так не
 * остаётся ни "следов" приглашения, ни хвостов старых символов.
 */
static void rl_redraw(void)
{
    uint32_t prompt_len = strlen(active_prompt);
    uint32_t i;

    console_putc('\r');
    for (i = printed_len; i > 0; i--) {
        console_putc(' ');
    }
    console_putc('\r');

    console_puts(active_prompt);
    printed_len = prompt_len;
    for (i = 0; i < line_len; i++) {
        console_putc(line_buf[i]);
        printed_len++;
    }

    /* аппаратный курсор — в позицию редактирования, а не в конец строки */
    console_set_cursor_col((uint8_t)(prompt_len + line_pos));
}

/* Печать prompt — вызывается в начале чтения */
static void rl_show_prompt(const char *prompt)
{
    active_prompt = prompt;
    console_puts(prompt);
    printed_len = strlen(prompt);
}

/* Вставка символа в позицию курсора */
static void rl_insert_char(char c)
{
    if (line_len >= READLINE_MAX_LEN) {
        return;
    }
    memmove(&line_buf[line_pos + 1], &line_buf[line_pos],
            line_len - line_pos);
    line_buf[line_pos] = c;
    line_len++;
    line_pos++;
    line_buf[line_len] = '\0';
}

/* Удаление символа слева от курсора (backspace) */
static void rl_backspace(void)
{
    if (line_pos == 0) {
        return;
    }
    memmove(&line_buf[line_pos - 1], &line_buf[line_pos],
            line_len - line_pos);
    line_len--;
    line_pos--;
    line_buf[line_len] = '\0';
}

/* Удаление символа под курсором (Delete) */
static void rl_delete(void)
{
    if (line_pos >= line_len) {
        return;
    }
    memmove(&line_buf[line_pos], &line_buf[line_pos + 1],
            line_len - line_pos - 1);
    line_len--;
    line_buf[line_len] = '\0';
}

/* Загрузка строки из истории в буфер редактирования */
static void rl_load_history(uint32_t idx)
{
    memset(line_buf, 0, READLINE_MAX_LEN + 1);
    if (idx < hist_count) {
        strcpy(line_buf, hist_lines[idx]);
    }
    line_len = strlen(line_buf);
    line_pos = line_len;
}

/* --- обработка ANSI-последовательностей ------------------------- */

/* ESC [ <...> — клавиши со стрелками и т.п. */
static void rl_handle_csi(void)
{
    char c = kbd_wait_char();

    switch (c) {
    case 'A':                       /* вверх: история назад */
        if (hist_cursor > 0) {
            if (hist_cursor == hist_count && line_len > 0) {
                strcpy(saved_line, line_buf);
                saved_line_valid = 1;
            }
            hist_cursor--;
            rl_load_history(hist_cursor);
        }
        break;
    case 'B':                       /* вниз: история вперёд */
        if (hist_cursor < hist_count) {
            hist_cursor++;
            if (hist_cursor == hist_count) {
                if (saved_line_valid) {
                    strcpy(line_buf, saved_line);
                } else {
                    line_buf[0] = '\0';
                }
                line_len = strlen(line_buf);
                line_pos = line_len;
            } else {
                rl_load_history(hist_cursor);
            }
        }
        break;
    case 'C':                       /* вправо */
        if (line_pos < line_len) {
            line_pos++;
        }
        break;
    case 'D':                       /* влево */
        if (line_pos > 0) {
            line_pos--;
        }
        break;
    case 'H':                       /* Home */
        line_pos = 0;
        break;
    case 'F':                       /* End */
        line_pos = line_len;
        break;
    case '3':                       /* Delete (ESC [ 3 ~) */
        if (kbd_wait_char() == '~') {
            rl_delete();
        }
        break;
    case '5':                       /* PageUp — игнорируем */
    case '6':                       /* PageDown — игнорируем */
        if (kbd_wait_char() == '~') {
            /* ничего */
        }
        break;
    default:
        break;
    }
}

static void rl_handle_escape(void)
{
    char c = kbd_wait_char();

    if (c == '[') {
        rl_handle_csi();
    }
    /* другие ESC-последовательности игнорируем */
}

/* Сохранение строки в историю (Enter) */
static void rl_save_history(void)
{
    if (line_len == 0) {
        return;
    }
    if (hist_count > 0 &&
        strcmp(hist_lines[hist_count - 1], line_buf) == 0) {
        return;                     /* не дублируем повторы */
    }
    if (hist_count < READLINE_HISTORY_SIZE) {
        strcpy(hist_lines[hist_count], line_buf);
        hist_count++;
    } else {
        /* кольцо: сдвигаем влево, новую пишем в конец */
        for (uint32_t i = 0; i < READLINE_HISTORY_SIZE - 1; i++) {
            strcpy(hist_lines[i], hist_lines[i + 1]);
        }
        strcpy(hist_lines[READLINE_HISTORY_SIZE - 1], line_buf);
    }
}

char *readline(const char *prompt)
{
    line_len = 0;
    line_pos = 0;
    line_buf[0] = '\0';
    hist_cursor = hist_count;
    saved_line_valid = 0;

    rl_show_prompt(prompt);

    for (;;) {
        char c = kbd_wait_char();

        switch (c) {
        case '\n':
        case '\r':
            console_putc('\n');
            rl_save_history();
            return line_buf;
        case '\b':
            rl_backspace();
            rl_redraw();
            break;
        case '\x1b':                /* ESC: ANSI-последовательность */
            rl_handle_escape();
            rl_redraw();
            break;
        default:
            if (c >= 0x20 && c < 0x7F) {
                /* редактирование строки сбрасывает навигацию по истории */
                hist_cursor = hist_count;
                saved_line_valid = 0;
                rl_insert_char(c);
                rl_redraw();
            }
            break;
        }
    }
}
