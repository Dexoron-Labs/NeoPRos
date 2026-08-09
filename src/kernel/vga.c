#include "kernel/vga.h"
#include "kernel/io.h"

/* Аппаратные константы VGA text mode (80x25, 16 цветов) */
#define VGA_MEMORY   ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH    80
#define VGA_HEIGHT   25

/* Индексы регистров контроллера дисплея (CRTC) */
#define VGA_CRTC_IDX 0x3D4
#define VGA_CRTC_DAT 0x3D5

/* Текущая позиция курсора (строка, колонка) */
static uint8_t vga_row;
static uint8_t vga_col;
static uint8_t vga_color;   /* атрибут: фон << 4 | цвет */

/* Возвращает линейный индекс позиции курсора: row * 80 + col */
static uint16_t vga_index(void)
{
    return (uint16_t)((uint16_t)vga_row * VGA_WIDTH + vga_col);
}

/* Записывает позицию курсора в регистры CRTC */
static void vga_update_cursor(void)
{
    uint16_t pos = vga_index();
    outb(VGA_CRTC_IDX, 14);
    outb(VGA_CRTC_DAT, (uint8_t)(pos >> 8));
    outb(VGA_CRTC_IDX, 15);
    outb(VGA_CRTC_DAT, (uint8_t)(pos & 0xFF));
}

/* Прокрутка экрана на одну строку вверх */
static void vga_scroll(void)
{
    volatile uint16_t *mem = VGA_MEMORY;

    for (uint16_t row = 1; row < VGA_HEIGHT; row++) {
        for (uint16_t col = 0; col < VGA_WIDTH; col++) {
            mem[(row - 1) * VGA_WIDTH + col] = mem[row * VGA_WIDTH + col];
        }
    }
    for (uint16_t col = 0; col < VGA_WIDTH; col++) {
        mem[(VGA_HEIGHT - 1) * VGA_WIDTH + col] =
            (uint16_t)(vga_color << 8 | ' ');
    }
}

void vga_init(void)
{
    vga_row = 0;
    vga_col = 0;
    vga_color = (uint8_t)(VGA_COLOR_BLACK << 4 | VGA_COLOR_WHITE);
    vga_clear();
}

void vga_clear(void)
{
    volatile uint16_t *mem = VGA_MEMORY;
    for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        mem[i] = (uint16_t)(vga_color << 8 | ' ');
    }
    vga_row = 0;
    vga_col = 0;
    vga_update_cursor();
}

void vga_set_color(uint8_t fg, uint8_t bg)
{
    vga_color = (uint8_t)(bg << 4 | fg);
}

void vga_set_cursor_col(uint8_t col)
{
    if (col >= VGA_WIDTH) {
        col = (uint8_t)(VGA_WIDTH - 1);
    }
    vga_col = col;
    vga_update_cursor();
}

void vga_putc(char c)
{
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\b') {
        /* backspace: на шаг назад и затирание символа */
        if (vga_col > 0) {
            vga_col--;
        }
        VGA_MEMORY[vga_index()] = (uint16_t)(vga_color << 8 | ' ');
    } else if (c == '\t') {
        do {
            vga_putc(' ');
        } while (vga_col % 4 != 0);
        return;
    } else {
        VGA_MEMORY[vga_index()] = (uint16_t)(vga_color << 8 | (uint8_t)c);
        vga_col++;
        if (vga_col == VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
    }

    if (vga_row == VGA_HEIGHT) {
        vga_scroll();
        vga_row = VGA_HEIGHT - 1;
    }
    vga_update_cursor();
}

void vga_puts(const char *str)
{
    while (*str) {
        vga_putc(*str++);
    }
}

void vga_print_hex32(uint32_t value)
{
    static const char hex[] = "0123456789abcdef";
    char buf[11];
    char *p = buf + sizeof(buf) - 1;

    *p = '\0';
    for (int i = 0; i < 8; i++) {
        *--p = hex[value & 0xF];
        value >>= 4;
    }
    *--p = 'x';
    *--p = '0';
    vga_puts(p);
}

void vga_print_dec32(uint32_t value)
{
    char buf[11];
    char *p = buf + sizeof(buf) - 1;

    *p = '\0';
    do {
        *--p = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);

    vga_puts(p);
}
