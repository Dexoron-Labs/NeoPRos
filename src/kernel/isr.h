#ifndef NEOPROS_ISR_H
#define NEOPROS_ISR_H

#include "multiboot.h"

/* Сохранённый контекст прерывания (layout в src/boot/isr_stubs.S) */
struct irq_regs {
    uint32_t ds, es, fs, gs;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t vector;
    uint32_t eip, cs, eflags;
};

/*
 * Единый диспетчер прерываний на C: вызывается из isr_common.
 *  - вектор < 32      — исключение CPU: паника с именем
 *  - вектор 32..47    — аппаратное прерывание IRQ0..IRQ15
 *  - остальное        — неиспользуемые векторы: паника
 */
void isr_dispatch(struct irq_regs *regs);

/* Векторы IRQ после remap PIC */
#define IRQ0_TIMER 32
#define IRQ1_KEYBOARD 33

#endif /* NEOPROS_ISR_H */
