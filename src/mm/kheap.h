#ifndef NEOPROS_KHEAP_H
#define NEOPROS_KHEAP_H

#include "kernel/multiboot.h"

/*
 * Куча ядра поверх физического менеджера памяти (pmm).
 * Простейший first-fit аллокатор: свободные блоки в связном
 * списке, при нехватке памяти куча расширяется новыми страницами.
 * Никаких зависимостей от libc.
 */
void kheap_init(uint32_t initial_bytes);

/* Выделение size байт; возвращает NULL при нехватке памяти. */
void *kmalloc(uint32_t size);

/* Освобождение блока, возвращённого kmalloc. */
void kfree(void *ptr);

/* Статистика: занято байт под данные и всего байт в куче. */
uint32_t kheap_used(void);
uint32_t kheap_total(void);

#endif /* NEOPROS_KHEAP_H */
