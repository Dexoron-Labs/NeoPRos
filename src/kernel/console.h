#ifndef NEOPROS_CONSOLE_H
#define NEOPROS_CONSOLE_H

#include "multiboot.h"
#include "vga.h"      /* VGA_COLOR_* — единая палитра */

/*
 * Единая консоль ядра: один и тот же текст с одинаковыми цветами
 * выводится и в VGA text mode, и в COM1 (с ANSI-последовательностями).
 * Тексты должны содержать только ASCII: VGA не понимает UTF-8.
 */
void console_init(void);

/* Установка цвета (одинаковая для VGA и ANSI-терминала). */
void console_set_color(uint8_t fg, uint8_t bg);

/* Вывод одного символа в обе консоли. */
void console_putc(char c);

/* Вывод строки в обе консоли. */
void console_puts(const char *str);

/* Вывод 32-битного числа в шестнадцатеричном виде (с префиксом 0x). */
void console_print_hex32(uint32_t value);

/* Вывод 32-битного беззнакового числа в десятичном виде. */
void console_print_dec32(uint32_t value);

#endif /* NEOPROS_CONSOLE_H */
