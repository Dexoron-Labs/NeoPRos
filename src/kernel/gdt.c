#include "gdt.h"

/* Селекторы сегментов ядра */
#define GDT_CODE_SEL 0x08    /* сегмент кода, индекс 1 */
#define GDT_DATA_SEL 0x10    /* сегмент данных, индекс 2 */

/* Массив дескрипторов GDT: 3 записи по 8 байт.
 *
 * Формат дескриптора (i386):
 *  биты 0-15   — лимит [15:0]
 *  биты 16-39  — база [23:0]
 *  биты 40-47  — байт доступа (P, DPL, S, тип)
 *  биты 48-51  — лимит [19:16]
 *  биты 52-55  — флаги (G, DB, L, AVL)
 *  биты 56-63  — база [31:24]
 */
static uint8_t gdt[3 * 8];

/* Псевдо-дескриптор для инструкции lgdt */
struct gdt_ptr {
    uint16_t limit;   /* размер GDT минус один */
    uint32_t base;    /* физический адрес GDT */
} __attribute__((packed));

/*
 * Собирает дескриптор:
 *   base     — линейный базовый адрес,
 *   limit    — лимит в байтах (0xFFFFF для 4 ГБ при гранулярности 4 КиБ),
 *   access   — байт доступа,
 *   granular — флаги гранулярности (0xCF — 4 КиБ гранулы + 32-битный).
 */
static void gdt_set_descriptor(uint8_t *desc, uint32_t base,
                               uint32_t limit, uint8_t access,
                               uint8_t granular)
{
    desc[0] = (uint8_t)(limit & 0xFF);
    desc[1] = (uint8_t)((limit >> 8) & 0xFF);
    desc[2] = (uint8_t)(base & 0xFF);
    desc[3] = (uint8_t)((base >> 8) & 0xFF);
    desc[4] = (uint8_t)((base >> 16) & 0xFF);
    desc[5] = access;
    desc[6] = (uint8_t)(((limit >> 16) & 0x0F) | granular);
    desc[7] = (uint8_t)((base >> 24) & 0xFF);
}

/* Выполняет far jump на следующий адрес — перезагружает CS. */
static void gdt_reload_segments(void)
{
    __asm__ volatile(
        "ljmp %0, $1f\n\t"
        "1:\n\t"
        "mov %1, %%ds\n\t"
        "mov %1, %%es\n\t"
        "mov %1, %%fs\n\t"
        "mov %1, %%gs\n\t"
        "mov %1, %%ss\n\t"
        :
        : "i"(GDT_CODE_SEL), "r"((uint32_t)GDT_DATA_SEL)
        : "memory");
}

void gdt_init(void)
{
    /* 0: нулевой дескриптор — обязателен по спецификации. */
    gdt_set_descriptor(&gdt[0], 0, 0, 0, 0);

    /* 1: плоский сегмент кода, 0..4 ГБ, 32-битный. */
    gdt_set_descriptor(&gdt[1 * 8], 0, 0xFFFFF,
                       0x9A,  /* present, кольцо 0, код, читаемый */
                       0xCF); /* 4 КиБ гранулы, 32-битный */

    /* 2: плоский сегмент данных, 0..4 ГБ, 32-битный. */
    gdt_set_descriptor(&gdt[2 * 8], 0, 0xFFFFF,
                       0x92,  /* present, кольцо 0, данные, записываемый */
                       0xCF); /* 4 КиБ гранулы, 32-битный */

    struct gdt_ptr ptr;
    ptr.limit = sizeof(gdt) - 1;
    ptr.base = (uint32_t)gdt;

    __asm__ volatile("lgdt %0" : : "m"(ptr) : "memory");

    gdt_reload_segments();
}
