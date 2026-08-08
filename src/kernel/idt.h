#ifndef NEOPROS_IDT_H
#define NEOPROS_IDT_H

#include "multiboot.h"

/* Количество векторов IDT (i386) */
#define IDT_ENTRIES 256

/*
 * Инициализация IDT: заполняет 256 шлюзов (32-битные interrupt
 * gates, DPL=0) на основе таблицы isr_stub_table и загружает IDTR.
 */
void idt_init(void);

/* Сброс обоих контроллеров 8259 (remap на 0x20/0x28). */
void pic_remap(void);

/* Разрешить/запретить конкретное IRQ в маске 8259. */
void pic_enable_irq(uint8_t irq);
void pic_disable_irq(uint8_t irq);

#endif /* NEOPROS_IDT_H */
