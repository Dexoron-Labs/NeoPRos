#include "mm/pmm.h"
#include "lib/string.h"
#include "kernel/console.h"

/* Символы из linker.ld: конец образа ядра (включая .bss и стек).
 * Загрузчик обнуляет память до _mb_bss_end, так что битовую карту
 * можно смело размещать сразу за этим адресом. */
extern char _mb_bss_end[];

/* Низший адрес, с которого начинается управляемая память (1 МиБ) */
#define PMM_BASE 0x100000u

/* Тип mmap-записи: доступная оперативная память */
#define MMAP_TYPE_RAM 1

/* Флаг multiboot_info: mmap_length/mmap_addr валидны (бит 6) */
#define MBI_FLAG_MMAP (1u << 6)

/*
 * Запись mmap. Обратите внимание: QEMU 11 (и новые версии) кладёт
 * адрес и длину как 64-битные значения (запись 24 байта), хотя
 * спецификация Multiboot 1 определяет их 32-битными. Проверено
 * hexdump'ом: size=20, addr 8 байт, len 8 байт, type 4 байта.
 * size по-прежнему не включает само поле size.
 */
struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
};

/* Битовая карта страниц */
static uint32_t *pmm_bitmap;
static uint32_t pmm_words;    /* число 32-битных слов карты */
static uint32_t pmm_pages;    /* число страниц в карте */
static uint32_t pmm_free;     /* число свободных страниц */

/* Битовые операции над картой */
static void pmm_bit_set(uint32_t index)
{
    pmm_bitmap[index / 32] |= (1u << (index % 32));
}

static void pmm_bit_clear(uint32_t index)
{
    pmm_bitmap[index / 32] &= ~(1u << (index % 32));
}

static uint32_t pmm_bit_test(uint32_t index)
{
    return (pmm_bitmap[index / 32] >> (index % 32)) & 1u;
}

/* Округление вверх до границы страницы */
static uint32_t pmm_align_up(uint32_t addr)
{
    return (addr + PMM_PAGE_SIZE - 1) & ~(PMM_PAGE_SIZE - 1);
}

/* Помечает страницы [addr, addr + size) как занятые.
 * Карта индексируется от PMM_BASE, поэтому first считается
 * относительно PMM_BASE (страница 0 карты = адрес PMM_BASE). */
static void pmm_reserve(uint32_t addr, uint32_t size)
{
    if (addr < PMM_BASE) {
        addr = PMM_BASE;
    }
    uint32_t first = (addr - PMM_BASE) / PMM_PAGE_SIZE;
    uint32_t pages = pmm_align_up(size) / PMM_PAGE_SIZE;
    uint32_t i;
    for (i = 0; i < pages; i++) {
        uint32_t idx = first + i;
        if (idx >= pmm_pages) {
            break;
        }
        pmm_bit_set(idx);
    }
}

/*
 * Строит битовую карту по mmap-записям Multiboot.
 * Все страницы изначально помечены занятыми; свободными
 * становятся только страницы RAM-областей (type == 1).
 * Ядро и сама карта исключаются из свободных.
 */
void pmm_init(const struct multiboot_info *mbi)
{
    /* 1. Верхняя граница памяти: максимум концов mmap-областей */
    uint32_t top = 0;
    uint32_t i;

    if (mbi->flags & MBI_FLAG_MMAP) {
        i = 0;
        while (i < mbi->mmap_length) {
            const struct multiboot_mmap_entry *entry =
                (const struct multiboot_mmap_entry *)(mbi->mmap_addr + i);
            if (entry->type == MMAP_TYPE_RAM) {
                uint64_t end64 = entry->addr + entry->len;
                if (end64 > top) {
                    top = (uint32_t)end64;
                }
            }
            i += entry->size + sizeof(entry->size);
        }
    } else {
        /* запасной вариант: верить полю mem_upper */
        top = PMM_BASE + mbi->mem_upper * 1024u;
    }
    if (top < PMM_BASE + PMM_PAGE_SIZE) {
        top = PMM_BASE + PMM_PAGE_SIZE;
    }

    /* 2. Размер карты и размещение сразу после ядра */
    pmm_pages = (top - PMM_BASE) / PMM_PAGE_SIZE;
    pmm_words = (pmm_pages + 31) / 32;
    pmm_bitmap = (uint32_t *)pmm_align_up((uint32_t)_mb_bss_end);

    /* все занято по умолчанию */
    memset(pmm_bitmap, 0xFF, pmm_words * 4);
    pmm_free = 0;

    /* 3. Свободными помечаем RAM-области из mmap */
    if (mbi->flags & MBI_FLAG_MMAP) {
        i = 0;
        while (i < mbi->mmap_length) {
            const struct multiboot_mmap_entry *entry =
                (const struct multiboot_mmap_entry *)(mbi->mmap_addr + i);
            if (entry->type == MMAP_TYPE_RAM) {
                uint32_t start = (uint32_t)entry->addr;
                uint32_t end = (uint32_t)(entry->addr + entry->len);

                if (start < PMM_BASE) {
                    start = PMM_BASE;
                }
                if (end > top) {
                    end = top;
                }
                if (end > start) {
                    uint32_t first = (start - PMM_BASE) / PMM_PAGE_SIZE;
                    uint32_t pages =
                        ((end - start) + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
                    for (uint32_t p = 0; p < pages && first + p < pmm_pages;
                         p++) {
                        pmm_bit_clear(first + p);
                        pmm_free++;
                    }
                }
            }
            i += entry->size + sizeof(entry->size);
        }
    } else {
        /* запасной вариант: вся память выше 1 МиБ свободна */
        uint32_t pages = (mbi->mem_upper * 1024u) / PMM_PAGE_SIZE;
        for (uint32_t p = 0; p < pages && p < pmm_pages; p++) {
            pmm_bit_clear(p);
            pmm_free++;
        }
    }

    /* 4. Ядро и битовая карта — занятые (защита от mmap-ошибок) */
    pmm_reserve(PMM_BASE, (uint32_t)_mb_bss_end - PMM_BASE);
    pmm_reserve((uint32_t)pmm_bitmap, pmm_words * 4);

    /* пересчёт свободных: заново не считаем, просто вычитаем
     * зарезервированное число страниц ядра и карты */
    pmm_free = pmm_pages;
    for (uint32_t p = 0; p < pmm_pages; p++) {
        if (pmm_bit_test(p)) {
            pmm_free--;
        }
    }
}

/*
 * Поиск непрерывного участка из n свободных страниц.
 * Линейный проход по словам карты, внутри — по битам.
 */
void *pmm_alloc_pages(uint32_t n)
{
    if (n == 0) {
        return NULL;
    }

    uint32_t run = 0;
    uint32_t start = 0;
    uint32_t p;

    for (p = 0; p < pmm_pages; p++) {
        if (pmm_bit_test(p)) {
            run = 0;
        } else {
            if (run == 0) {
                start = p;
            }
            run++;
            if (run == n) {
                for (uint32_t q = start; q < start + n; q++) {
                    pmm_bit_set(q);
                }
                pmm_free -= n;
                return (void *)(PMM_BASE + start * PMM_PAGE_SIZE);
            }
        }
    }
    return NULL;    /* нет непрерывного участка */
}

void pmm_free_pages(void *addr, uint32_t n)
{
    uint32_t base = (uint32_t)addr;

    if (base < PMM_BASE || base >= PMM_BASE + pmm_pages * PMM_PAGE_SIZE ||
        n == 0) {
        return;
    }

    uint32_t first = (base - PMM_BASE) / PMM_PAGE_SIZE;
    for (uint32_t p = 0; p < n && first + p < pmm_pages; p++) {
        if (pmm_bit_test(first + p)) {
            pmm_bit_clear(first + p);
            pmm_free++;
        }
    }
}

void pmm_reserve_range(uint32_t addr, uint32_t size)
{
    pmm_reserve(addr, size);
}

uint32_t pmm_free_page_count(void)
{
    return pmm_free;
}

uint32_t pmm_total_page_count(void)
{
    return pmm_pages;
}
