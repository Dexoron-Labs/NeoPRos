#ifndef NEOPROS_KBD_H
#define NEOPROS_KBD_H

#include "multiboot.h"

/* Размер кольцевого буфера сканкодов */
#define KBD_BUFFER_SIZE 128

/*
 * Инициализация клавиатуры PS/2: сброс контроллера 8042,
 * разрешение IRQ1, очистка буфера.
 */
void kbd_init(void);

/* Получение символа из буфера; -1, если буфер пуст. */
int kbd_getc(void);

/* Ожидание символа (блокирующая). */
char kbd_wait_char(void);

/* Число символов в буфере. */
uint32_t kbd_available(void);

#endif /* NEOPROS_KBD_H */
