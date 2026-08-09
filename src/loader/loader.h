#ifndef NEOPROS_LOADER_H
#define NEOPROS_LOADER_H

#include "kernel/multiboot.h"

/*
 * Загрузчик программ .BIN.
 *
 * Программа — плоский 32-битный бинарник (clang + lld с
 * -Ttext=0x400000). Вход: void _start(void *api, int argc, char **argv);
 * выход — обычный ret. Запуск происходит синхронно в контексте ядра
 * (ring 0, без paging); после завершения управление возвращается
 * вызывающему.
 */

/* База и максимальный размер образа программы. */
#define PROGRAM_BASE 0x400000u
#define PROGRAM_MAX  0x0E0000u     /* 896 КиБ */

/* Стек программы: 64 КиБ от PROGRAM_STACK_BOTTOM до PROGRAM_STACK_TOP. */
#define PROGRAM_STACK_BOTTOM 0x4F0000u
#define PROGRAM_STACK_TOP    0x500000u

/* Резервирование зоны программ в физической памяти (после pmm_init). */
void loader_init(void);

/*
 * Загрузка и запуск программы name (файл на RAM-диске).
 * args — командная строка (может быть NULL). Возвращает код
 * завершения программы: 0 — нормальный выход (ret), иначе код,
 * переданный sysapi.exit(). -1 — ошибка загрузки.
 */
int loader_run(const char *name, const char *args);

/* Завершение текущей программы с кодом code (не возвращается). */
void loader_exit(int code);

#endif /* NEOPROS_LOADER_H */
