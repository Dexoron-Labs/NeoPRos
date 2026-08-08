#include "kbd.h"
#include "io.h"
#include "string.h"

/* --- Таблицы сканкодов (set 1) ------------------------------- */
/* Обычные символы; 0 = нет символа (служебные клавиши) */
static const char kbd_map_base[128] = {
    0, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    0x08, 0x09, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Символы при нажатом Shift */
static const char kbd_map_shift[128] = {
    0, 0x1B, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    0x08, 0x09, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Кольцевой буфер символов */
static volatile char buffer[KBD_BUFFER_SIZE];
static volatile uint32_t buf_head;
static volatile uint32_t buf_tail;

/* Состояние модификаторов */
static volatile uint8_t shift_pressed;
static volatile uint8_t extended_pending;
static volatile uint8_t break_pending;

/* Сканкоды модификаторов (set 1) */
#define SCAN_LSHIFT 0x2A
#define SCAN_RSHIFT 0x36

/* Служебные коды потока клавиатуры */
#define SCAN_PREFIX_EXT 0xE0   /* префикс расширенных клавиш */
#define SCAN_PREFIX_BRK 0xF0   /* префикс релиза (set 2 / QEMU) */

/* Ожидание готовности контроллера 8042 к приёму команды */
static void kbd_wait_input(void)
{
    while (inb(0x64) & 0x02) {
    }
}

/* Обработчик IRQ1: вызывается из isr_dispatch.
 * Принимает поток как с реального 8042 (set 1: make = код,
 * break = код|0x80), так и от QEMU (make = код set 1, break
 * в стиле set 2: префикс 0xF0 + код). */
void irq1_keyboard(void)
{
    uint8_t scancode = inb(0x60);

    if (scancode == SCAN_PREFIX_EXT) {  /* префикс расширенных клавиш */
        extended_pending = 1;
        return;
    }
    if (extended_pending) {             /* расширенные клавиши пока игнорируем */
        extended_pending = 0;
        return;
    }
    if (scancode == SCAN_PREFIX_BRK) {  /* префикс релиза (стиль set 2) */
        break_pending = 1;
        return;
    }

    if (break_pending) {                /* релиз с префиксом 0xF0 */
        break_pending = 0;
        scancode &= 0x7F;
        if (scancode == SCAN_LSHIFT || scancode == SCAN_RSHIFT) {
            shift_pressed = 0;
        }
        return;
    }

    if (scancode & 0x80) {              /* отпускание клавиши (set 1) */
        scancode &= 0x7F;
        if (scancode == SCAN_LSHIFT || scancode == SCAN_RSHIFT) {
            shift_pressed = 0;
        }
        return;
    }

    switch (scancode) {
    case SCAN_LSHIFT:
    case SCAN_RSHIFT:
        shift_pressed = 1;
        return;
    default:
        break;
    }

    if (scancode >= 128) {
        return;
    }

    char c = shift_pressed ? kbd_map_shift[scancode] : kbd_map_base[scancode];
    if (c == 0) {
        return;
    }

    uint32_t next = (buf_head + 1) % KBD_BUFFER_SIZE;
    if (next == buf_tail) {
        return;                     /* буфер переполнен: теряем символ */
    }
    buffer[buf_head] = c;
    buf_head = next;
}

void kbd_init(void)
{
    buf_head = 0;
    buf_tail = 0;
    shift_pressed = 0;
    extended_pending = 0;
    break_pending = 0;

    /* очищаем выходной буфер контроллера */
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }

    /* NB: набор сканкодов не переключаем (команда 0xF0)!
     * На реальном железе клавиатура по умолчанию работает в
     * set 2, а контроллер 8042 аппаратно транслирует его в set 1.
     * QEMU 11 в этом режиме шлёт make-коды set 1 и релизы
     * в стиле set 2 (0xF0 + код) — драйвер принимает оба стиля. */

    /* сброс контроллера 8042 */
    kbd_wait_input();
    outb(0x64, 0xAE);               /* разрешить интерфейс клавиатуры */
}

int kbd_getc(void)
{
    if (buf_head == buf_tail) {
        return -1;
    }
    char c = buffer[buf_tail];
    buf_tail = (buf_tail + 1) % KBD_BUFFER_SIZE;
    return (int)(uint8_t)c;
}

char kbd_wait_char(void)
{
    int c;
    while ((c = kbd_getc()) == -1) {
        __asm__ volatile("hlt");
    }
    return (char)c;
}

uint32_t kbd_available(void)
{
    return (buf_head - buf_tail) % KBD_BUFFER_SIZE;
}
