#include "loader/loader.h"
#include "fs/fs.h"
#include "mm/pmm.h"
#include "api/sysapi.h"
#include "lib/string.h"

/*
 * Загрузчик .BIN. Программа грузится по PROGRAM_BASE, её стек —
 * PROGRAM_STACK_TOP. Аргументы командной строки строятся в верхней
 * странице стека (argv-массив и строки), так что программа с
 * разумным потреблением стека (< 60 КиБ) не заденет их.
 *
 * Выход программы — обычный ret: ядро кладёт на стек программы
 * адрес возврата и переходит в entry. Выход через sysapi.exit()
 * реализован setjmp/longjmp на уровне загрузчика.
 */

/* Область построения argv: верхняя страница стека программы. */
#define ARGV_BASE   (PROGRAM_STACK_BOTTOM)
#define ARGV_LIMIT  (PROGRAM_STACK_BOTTOM + 0x1000u)
#define ARGV_MAX    32

/* Контекст для sysapi.exit(): setjmp/longjmp-буфер загрузчика.
 * exit_env — глобальная (не static), т.к. на неё ссылается
 * inline-ассемблер loader_run по имени символа. */
uint32_t exit_env[6];
static uint32_t exit_code;

/* --- setjmp/longjmp: [0]=ebx [1]=esi [2]=edi [3]=ebp [4]=esp [5]=eip ---
 * Реализация в loader_asm.S: кадр фиксирован. */

extern uint32_t loader_setjmp(uint32_t env[6]);
extern void loader_longjmp(uint32_t env[6], int value);

/* --- построение argv ------------------------------------------------ */

/*
 * Копирует командную строку args в область argv, разбивая на токены
 * (пробелы/табы, кавычки " ' и экранирование \). argv[0] — имя файла.
 * Возвращает argc или -1 при переполнении буфера.
 */
static int build_argv(const char *name, const char *args)
{
    char **argv = (char **)ARGV_BASE;
    char *str = (char *)(ARGV_BASE + 128);
    char *end = (char *)ARGV_LIMIT;
    int argc = 0;

    uint32_t nlen = strlen(name);
    if ((uint32_t)(end - str) <= nlen) {
        return -1;
    }
    strcpy(str, name);
    argv[argc++] = str;
    str += nlen + 1;

    if (args != NULL) {
        const char *src = args;
        while (*src && argc < ARGV_MAX) {
            while (*src == ' ' || *src == '\t') {
                src++;
            }
            if (*src == '\0') {
                break;
            }
            if (end - str < 2) {
                return -1;
            }
            argv[argc++] = str;
            char quote = 0;
            while (*src) {
                char c = *src++;
                if (quote) {
                    if (c == quote) {
                        quote = 0;
                    } else {
                        if (end - str <= 0) {
                            return -1;
                        }
                        *str++ = c;
                    }
                    continue;
                }
                if (c == '"' || c == '\'') {
                    quote = c;
                    continue;
                }
                if (c == '\\' && *src) {
                    if (end - str <= 0) {
                        return -1;
                    }
                    *str++ = *src++;
                    continue;
                }
                if (c == ' ' || c == '\t') {
                    break;
                }
                if (end - str <= 0) {
                    return -1;
                }
                *str++ = c;
            }
            *str++ = '\0';
        }
    }
    argv[argc] = NULL;
    return argc;
}

/* --- публичное API --------------------------------------------------- */

void loader_init(void)
{
    /* вся зона программ (код + стек) недоступна менеджеру памяти */
    pmm_reserve_range(PROGRAM_BASE, PROGRAM_STACK_TOP - PROGRAM_BASE);
}

void loader_exit(int code)
{
    exit_code = (uint32_t)code;
    loader_longjmp(exit_env, 1);
    /* не возвращается */
}

int loader_run(const char *name, const char *args)
{
    uint32_t size = fs_size(name);
    if (size == 0 || size > PROGRAM_MAX) {
        return -1;
    }
    if (fs_load(name, (void *)PROGRAM_BASE, PROGRAM_MAX) != size) {
        return -1;
    }

    int argc = build_argv(name, args);
    if (argc < 0) {
        return -1;
    }

    struct neopros_api *api = sysapi_get();
    char **argv = (char **)ARGV_BASE;
    void (*entry)(void *, int, char **) =
        (void (*)(void *, int, char **))(uint32_t)PROGRAM_BASE;

    /* регистрируем точку возврата для sysapi.exit() */
    exit_code = 0;
    if (loader_setjmp(exit_env) != 0) {
        /* сюда возвращает exit(): esp уже восстановлен */
        return (int)exit_code;
    }

    /* передача управления: cdecl-фрейм кладётся на стек программы,
     * адрес возврата — чтобы ret программы вернул в ядро.
     * После ret программа не может продолжать код компилятора
     * (кадр loader_run разрушен: esp/ebp указывают на стек программы),
     * поэтому возврат идёт через полное восстановление контекста
     * из exit_env — как в loader_longjmp, со значением 1 (не 0). */
    __asm__ volatile(
        "movl %0, %%esp\n\t"
        "pushl %3\n\t"               /* argv */
        "pushl %2\n\t"               /* argc */
        "pushl %1\n\t"               /* api */
        "pushl $1f\n\t"              /* адрес возврата в ядро */
        "jmp *%4\n\t"
        "1:\n\t"
        "movl $1, %%eax\n\t"         /* value != 0 для setjmp-семантики */
        "leal exit_env, %%ecx\n\t"
        "movl 0(%%ecx), %%ebx\n\t"
        "movl 4(%%ecx), %%esi\n\t"
        "movl 8(%%ecx), %%edi\n\t"
        "movl 12(%%ecx), %%ebp\n\t"
        "movl 16(%%ecx), %%esp\n\t"
        "jmp *20(%%ecx)\n\t"
        :
        : "i"((uint32_t)PROGRAM_STACK_TOP), "m"(api), "m"(argc),
          "m"(argv), "m"(entry)
        : "eax", "ebx", "ecx", "esi", "edi", "ebp", "esp", "memory");
    __builtin_unreachable();
}
