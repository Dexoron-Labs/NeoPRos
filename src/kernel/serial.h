#ifndef NEOPROS_SERIAL_H
#define NEOPROS_SERIAL_H

#include "kernel/multiboot.h"

/* Инициализация COM1 (38400 бод, 8N1). */
void com1_init(void);

/* Установка цвета в ANSI-терминале (те же значения, что у VGA). */
void com1_set_color(uint8_t fg, uint8_t bg);

/* Вывод одного символа в COM1. */
void com1_putc(char c);

/* Вывод строки в COM1. */
void com1_puts(const char *str);

/* Вывод 32-битного числа в шестнадцатеричном виде (с префиксом 0x). */
void com1_print_hex32(uint32_t value);

/* Вывод 32-битного беззнакового числа в десятичном виде. */
void com1_print_dec32(uint32_t value);

#endif /* NEOPROS_SERIAL_H */
