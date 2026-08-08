#include "serial.h"
#include "io.h"

/* Регистры COM1 (порт базы 0x3F8) */
#define COM1_THR 0x3F8   /* передающий регистр (Transmit Holding) */
#define COM1_IER 0x3F9   /* регистр разрешения прерываний */
#define COM1_LCR 0x3FB   /* регистр управления линией */
#define COM1_MCR 0x3FC   /* регистр управления модемом */
#define COM1_LSR 0x3FD   /* регистр состояния линии */
#define COM1_DLL 0x3F8   /* делитель, младший байт (при DLAB=1) */
#define COM1_DLM 0x3F9   /* делитель, старший байт (при DLAB=1) */

void com1_init(void)
{
    outb(COM1_IER, 0x00);        /* запрещаем все прерывания UART */
    outb(COM1_LCR, 0x80);        /* DLAB = 1 — доступ к делителю */
    outb(COM1_DLL, 0x03);        /* делитель 3 → 38400 бод */
    outb(COM1_DLM, 0x00);
    outb(COM1_LCR, 0x03);        /* DLAB = 0, 8 бит, без чётности, 1 стоп */
    outb(COM1_MCR, 0x03);        /* DTR и RTS активны */
}

/* VGA-цвет (0..15) → ANSI-код текста: 30..37 (обычные) или 90..97 (яркие) */
static uint8_t ansi_fg(uint8_t vga_color)
{
    return (uint8_t)((vga_color & 0x8) ? 90 + (vga_color & 0x7)
                                       : 30 + vga_color);
}

/* VGA-цвет (0..15) → ANSI-код фона: 40..47 или 100..107 */
static uint8_t ansi_bg(uint8_t vga_color)
{
    return (uint8_t)((vga_color & 0x8) ? 100 + (vga_color & 0x7)
                                       : 40 + vga_color);
}

void com1_set_color(uint8_t fg, uint8_t bg)
{
    /* Escape-последовательность: ESC [ <fg> ; <bg> m */
    com1_putc('\x1b');
    com1_putc('[');
    com1_print_dec32(ansi_fg(fg));
    com1_putc(';');
    com1_print_dec32(ansi_bg(bg));
    com1_putc('m');
}

void com1_putc(char c)
{
    /* ждём, пока передающий буфер освободится (бит 5 регистра LSR) */
    while ((inb(COM1_LSR) & 0x20) == 0) {
    }
    outb(COM1_THR, (uint8_t)c);
}

void com1_puts(const char *str)
{
    while (*str) {
        com1_putc(*str++);
    }
}

void com1_print_hex32(uint32_t value)
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
    com1_puts(p);
}

void com1_print_dec32(uint32_t value)
{
    char buf[11];
    char *p = buf + sizeof(buf) - 1;

    *p = '\0';
    do {
        *--p = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);

    com1_puts(p);
}
