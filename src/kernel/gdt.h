#ifndef NEOPROS_GDT_H
#define NEOPROS_GDT_H

#include "kernel/multiboot.h"

/*
 * Минимальная GDT ядра: нулевой дескриптор + два плоских
 * сегмента (код и данные, 0..4 ГБ). Устанавливается сразу
 * после входа в ядро, чтобы не зависеть от GDT загрузчика.
 */
void gdt_init(void);

#endif /* NEOPROS_GDT_H */
