#include "console.h"
#include "vga.h"
#include "serial.h"

#include <stdarg.h>

void console_init(void)
{
    com1_init();
    vga_init();
}

void console_clear(void)
{
    vga_clear();
    com1_puts("\x1b[2J\x1b[H");
}

void console_set_color(uint8_t fg, uint8_t bg)
{
    vga_set_color(fg, bg);
    com1_set_color(fg, bg);
}

void console_set_cursor_col(uint8_t col)
{
    vga_set_cursor_col(col);
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

/*
 * Упрощённый форматтер для ядра: %s %c %d %u %x %%.
 * Поддерживает нулевую ширину вида %02u (только '0', без '*').
 */
static void kvformat(char *buf, const char *fmt, va_list ap)
{
    char *p = buf;

    while (*fmt) {
        if (*fmt != '%') {
            *p++ = *fmt++;
            continue;
        }
        fmt++;

        /* необязательная ширина: %0Nd — дополнение нулями до N знаков */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (s == NULL) {
                s = "(null)";
            }
            while (*s) {
                *p++ = *s++;
            }
            break;
        }
        case 'c':
            *p++ = (char)va_arg(ap, int);
            break;
        case 'd': {
            int32_t v = va_arg(ap, int32_t);
            char tmp[12];
            char *q = tmp + sizeof(tmp) - 1;
            *q = '\0';
            uint32_t u = (v < 0) ? (uint32_t)(-v - 1) + 1 : (uint32_t)v;
            int ndigits = 0;
            do {
                *--q = (char)('0' + u % 10);
                u /= 10;
                ndigits++;
            } while (u != 0);
            if (v < 0) {
                *--q = '-';
            }
            while (ndigits < width) {
                *p++ = '0';
                ndigits++;
            }
            while (*q) {
                *p++ = *q++;
            }
            break;
        }
        case 'u': {
            uint32_t v = va_arg(ap, uint32_t);
            char tmp[12];
            char *q = tmp + sizeof(tmp) - 1;
            *q = '\0';
            int ndigits = 0;
            do {
                *--q = (char)('0' + v % 10);
                v /= 10;
                ndigits++;
            } while (v != 0);
            while (ndigits < width) {
                *p++ = '0';
                ndigits++;
            }
            while (*q) {
                *p++ = *q++;
            }
            break;
        }
        case 'x': {
            static const char hex[] = "0123456789abcdef";
            uint32_t v = va_arg(ap, uint32_t);
            char tmp[12];
            char *q = tmp + sizeof(tmp) - 1;
            *q = '\0';
            int ndigits = 0;
            do {
                *--q = hex[v & 0xF];
                v >>= 4;
                ndigits++;
            } while (v != 0);
            while (ndigits < width) {
                *p++ = '0';
                ndigits++;
            }
            while (*q) {
                *p++ = *q++;
            }
            break;
        }
        case '%':
            *p++ = '%';
            break;
        default:
            *p++ = '%';
            if (*fmt) {
                *p++ = *fmt;
            }
            break;
        }
        if (*fmt) {
            fmt++;
        }
    }
    *p = '\0';
}

void kprintf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    kvformat(buf, fmt, ap);
    va_end(ap);

    console_puts(buf);
}
