#ifndef NEOPROS_VGA_H
#define NEOPROS_VGA_H

#include "kernel/multiboot.h"

/*
 * Цвета VGA (верхняя половина байта атрибута — фон, нижняя — текст).
 * Значения — как у x16-PRos (kernel.asm COLOR_*): яркие цвета,
 * белый 0x0F; реальные оттенки задаёт палитра DEFAULT.THM (vga.c).
 */
#define VGA_COLOR_BLACK    0x0
#define VGA_COLOR_GREEN    0xA
#define VGA_COLOR_CYAN     0xB
#define VGA_COLOR_RED      0xC
#define VGA_COLOR_WHITE    0xF
#define VGA_COLOR_YELLOW   0xE
#define VGA_COLOR_MAGENTA  0xD

/* Инициализация: очистка экрана, сброс курсора. */
void vga_init(void);

/* Очистка экрана с сохранением текущего цвета, курсор в 0;0. */
void vga_clear(void);

/* Установка цвета текста и фона. */
void vga_set_color(uint8_t fg, uint8_t bg);

/* Перемещение аппаратного курсора в колонку на текущей строке.
 * Используется редактором строки при перерисовке. */
void vga_set_cursor_col(uint8_t col);

/* Вывод одного символа с учётом курсора и прокрутки. */
void vga_putc(char c);

/* Вывод строки в VGA. */
void vga_puts(const char *str);

/* Вывод 32-битного числа в шестнадцатеричном виде (с префиксом 0x). */
void vga_print_hex32(uint32_t value);

/* Вывод 32-битного беззнакового числа в десятичном виде. */
void vga_print_dec32(uint32_t value);

#endif /* NEOPROS_VGA_H */
