#include "kernel/pit.h"
#include "kernel/io.h"

/* Базовый счётчик PIT: 1193182 Гц */
#define PIT_BASE_FREQ 1193182

/* Счётчик тиков с момента загрузки */
static volatile uint32_t pit_tick_count;

void pit_init(void)
{
    pit_tick_count = 0;

    uint32_t divisor = PIT_BASE_FREQ / PIT_HZ;

    /* канал 0, lo/hi байты, режим 3, двоичный */
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8));
}

/* Обработчик IRQ0: вызывается из isr_dispatch */
void irq0_timer(void)
{
    pit_tick_count++;
}

uint32_t pit_ticks(void)
{
    return pit_tick_count;
}
