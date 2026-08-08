#include "console.h"
#include "vga.h"
#include "serial.h"

void console_init(void)
{
    com1_init();
    vga_init();
}

void console_set_color(uint8_t fg, uint8_t bg)
{
    vga_set_color(fg, bg);
    com1_set_color(fg, bg);
}

void console_putc(char c)
{
    vga_putc(c);
    com1_putc(c);
}

void console_puts(const char *str)
{
    vga_puts(str);
    com1_puts(str);
}

void console_print_hex32(uint32_t value)
{
    vga_print_hex32(value);
    com1_print_hex32(value);
}

void console_print_dec32(uint32_t value)
{
    vga_print_dec32(value);
    com1_print_dec32(value);
}
