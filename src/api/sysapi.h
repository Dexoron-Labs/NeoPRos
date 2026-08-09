#ifndef NEOPROS_SYSAPI_H
#define NEOPROS_SYSAPI_H

#include "kernel/multiboot.h"

/*
 * Системный API NeoPRos (32-битный ABI).
 *
 * Ядро держит таблицу указателей (см. sysapi.c) и публикует её:
 *   1) адрес таблицы передаётся программе первым аргументом _start(),
 *   2) указатель на неё дополнительно пишется по адресу
 *      NEOPROS_API_PTR_ADDR (если туда не легла структура multiboot_info).
 *
 * Программа .BIN — плоский 32-битный бинарник, собирается clang + lld
 * с -Ttext=0x400000. Вход: void _start(void *api, int argc, char **argv);
 * выход — обычный ret. Программа работает в ring 0 без paging.
 */

/* Адрес, по которому публикуется указатель на таблицу API. */
#define NEOPROS_API_PTR_ADDR 0x10000u

/* Магическое число и версия ABI таблицы. */
#define NEOPROS_API_MAGIC 0x4E505231u /* "NPR1" */
#define NEOPROS_API_VERSION 0x0100u

/* Дата/время из RTC (CMOS), значения двоичные. */
struct neopros_time {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

/* Системная информация. */
struct neopros_sysinfo {
    uint32_t version;        /* NEOPROS_API_VERSION */
    uint32_t mem_total;      /* физическая память, КиБ */
    uint32_t mem_free;       /* свободная память, КиБ */
    uint32_t uptime;         /* аптайм системы, секунды */
};

/*
 * Таблица функций ядра. Структура стабильна: новые функции добавляются
 * только в конец, порядок полей не меняется.
 */
struct neopros_api {
    uint32_t magic;
    uint32_t version;

    /* --- вывод -------------------------------------------- */
    void (*puts)(const char *str);
    void (*putc)(char c);
    void (*printf)(const char *fmt, ...);   /* формат, как у ядра */
    void (*cls)(void);
    void (*set_color)(uint8_t fg, uint8_t bg);

    /* --- ввод ---------------------------------------------- */
    int (*getchar)(void);                   /* -1, если буфер пуст */
    void (*readline)(const char *prompt, char *buf, uint32_t size);

    /* --- файловая система (FAT12, read-only) --------------- */
    /* Загрузить файл целиком в buf (не более size байт).
     * Возвращает число прочитанных байт или 0 при ошибке. */
    uint32_t (*fs_load)(const char *name, void *buf, uint32_t size);
    /* Список каталога в буфер: строки "NAME.EXT SIZE\n", 0 в конце.
     * Возвращает число записей или 0xFFFFFFFF при ошибке. */
    uint32_t (*fs_list)(const char *dir, char *buf, uint32_t size);
    /* Проверка существования файла/каталога (1 или 0). */
    int (*fs_exists)(const char *name);
    /* Размер файла в байтах (0xFFFFFFFF при ошибке). */
    uint32_t (*fs_size)(const char *name);
    /* Смена текущего каталога на один компонент (0 ок, -1 ошибка). */
    int (*fs_chdir)(const char *dir);
    /* Текущий каталог, "/" для корня. */
    void (*fs_getcwd)(char *buf, uint32_t size);

    /* --- системная информация ------------------------------ */
    void (*sysinfo)(struct neopros_sysinfo *info);
    void (*rtc)(struct neopros_time *t);

    /* --- запуск программ ------------------------------------ */
    /* Запустить программу .BIN с командной строкой args
     * (или NULL). Возвращает код завершения программы. */
    int (*exec)(const char *name, const char *args);
    /* Завершить текущую программу с кодом (не возвращается). */
    void (*exit)(int code);
};

/*
 * Получение таблицы API. Для программ обычно достаточно аргумента
 * _start(), но указатель также можно прочитать по адресу
 * NEOPROS_API_PTR_ADDR.
 */
static inline struct neopros_api *neopros_api(void)
{
    return *(struct neopros_api **)NEOPROS_API_PTR_ADDR;
}

/* --- только для ядра ------------------------------------------------ */

/* Публикация указателя на таблицу (см. sysapi.c). */
void sysapi_init(uint32_t mbi_addr);

/* Сама таблица (передаётся программе аргументом _start). */
struct neopros_api *sysapi_get(void);

#endif /* NEOPROS_SYSAPI_H */
