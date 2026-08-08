#ifndef NEOPROS_PMM_H
#define NEOPROS_PMM_H

#include "multiboot.h"

/* Размер страницы физической памяти */
#define PMM_PAGE_SIZE 4096

/*
 * Физический менеджер памяти (Physical Memory Manager).
 * Работает с битовой картой: один бит на страницу 4 КиБ.
 * Карта строится по mmap-записям Multiboot, поэтому учитывает
 * зарезервированные области (ACPI и т.п.) загрузчика.
 */
void pmm_init(const struct multiboot_info *mbi);

/* Выделение n подряд идущих страниц; возвращает физический адрес
 * или NULL, если непрерывного участка не нашлось. */
void *pmm_alloc_pages(uint32_t n);

/* Освобождение n страниц, начиная с физического адреса addr. */
void pmm_free_pages(void *addr, uint32_t n);

/* Число свободных и общее число страниц в карте. */
uint32_t pmm_free_page_count(void);
uint32_t pmm_total_page_count(void);

#endif /* NEOPROS_PMM_H */
