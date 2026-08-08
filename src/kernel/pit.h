#ifndef NEOPROS_PIT_H
#define NEOPROS_PIT_H

#include "multiboot.h"

/* Частота системного таймера, Гц */
#define PIT_HZ 100

/*
 * Инициализация канала 0 PIT (8253/8254) в режиме 3 (square wave).
 * Таймер обнуляет tick counter, IRQ0 разрешён в pic_enable_irq.
 */
void pit_init(void);

/* Число тиков с момента загрузки (инкрементируется в IRQ0). */
uint32_t pit_ticks(void);

#endif /* NEOPROS_PIT_H */
