#ifndef NEOPROS_VGA_H
#define NEOPROS_VGA_H

#include "multiboot.h"

/* Цвета VGA (верхняя половина байта атрибута — фон, нижняя — текст) */
#define VGA_COLOR_BLACK   0x0
#define VGA_COLOR_GREEN   0x2
#define VGA_COLOR_CYAN    0x3
#define VGA_COLOR_WHITE   0x7

/* Инициализация: очистка экрана, сброс курсора. */
void vga_init(void);

/* Установка цвета текста и фона. */
void vga_set_color(uint8_t fg, uint8_t bg);

/* Вывод одного символа с учётом курсора и прокрутки. */
void vga_putc(char c);

/* Вывод строки в VGA. */
void vga_puts(const char *str);

/* Вывод 32-битного числа в шестнадцатеричном виде (с префиксом 0x). */
void vga_print_hex32(uint32_t value);

/* Вывод 32-битного беззнакового числа в десятичном виде. */
void vga_print_dec32(uint32_t value);

#endif /* NEOPROS_VGA_H */
