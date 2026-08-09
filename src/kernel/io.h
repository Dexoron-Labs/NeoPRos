#ifndef NEOPROS_IO_H
#define NEOPROS_IO_H

#include "kernel/multiboot.h"

/* Отправка байта в порт ввода-вывода (i386: инструкция OUT). */
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Отправка 16-битного слова в порт (i386: инструкция OUTW). */
static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/* Чтение байта из порта ввода-вывода (i386: инструкция IN). */
static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* Пауза на 1..65535 циклов ввода-вывода (ожидание порта). */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

#endif /* NEOPROS_IO_H */
