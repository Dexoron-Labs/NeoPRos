#include "mm/kheap.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include "kernel/io.h"

/* Выравнивание блоков: заголовок и данные кратны 8 байтам */
#define KHEAP_ALIGN 8

/* Заголовок блока кучи */
struct kheap_block {
    uint32_t size;              /* размер данных блока (без заголовка) */
    uint32_t free;              /* 1 — блок свободен */
    struct kheap_block *next;   /* следующий блок */
};

/* При нехватке памяти куча расширяется минимум на столько байт */
#define KHEAP_GROW_MIN (128u * 1024u)

static struct kheap_block *kheap_head;
static struct kheap_block *kheap_tail;   /* последний блок (для расширения) */
static uint32_t kheap_total_bytes;
static uint32_t kheap_used_bytes;

static uint32_t kheap_align_up(uint32_t value)
{
    return (value + KHEAP_ALIGN - 1) & ~(KHEAP_ALIGN - 1);
}

/*
 * Расширение кучи: выделяем непрерывные страницы у pmm и
 * добавляем их к последнему блоку. Если хвост свободен,
 * просто увеличиваем его размер — блоки остаются упорядоченными.
 */
static int kheap_grow(uint32_t bytes)
{
    uint32_t pages = kheap_align_up(bytes) / PMM_PAGE_SIZE;
    void *mem = pmm_alloc_pages(pages);

    if (mem == NULL) {
        return -1;
    }

    struct kheap_block *nb = (struct kheap_block *)mem;
    nb->size = pages * PMM_PAGE_SIZE - sizeof(struct kheap_block);
    nb->free = 1;
    nb->next = NULL;

    if (kheap_tail->free) {
        kheap_tail->size += nb->size + sizeof(struct kheap_block);
    } else {
        kheap_tail->next = nb;
        kheap_tail = nb;
    }
    kheap_total_bytes += pages * PMM_PAGE_SIZE;
    return 0;
}

void kheap_init(uint32_t initial_bytes)
{
    uint32_t pages = kheap_align_up(initial_bytes) / PMM_PAGE_SIZE;
    if (pages == 0) {
        pages = 1;
    }

    void *mem = pmm_alloc_pages(pages);
    if (mem == NULL) {
        /* нехватка физической памяти: паника с сообщением в COM1 */
        static const char msg[] = "\r\nkheap: no memory for initial heap!\r\n";
        const char *p = msg;
        while (*p) {
            outb(0x3F8, (uint8_t)*p++);
        }
        for (;;) {
        }
    }

    kheap_head = (struct kheap_block *)mem;
    kheap_head->size = pages * PMM_PAGE_SIZE - sizeof(struct kheap_block);
    kheap_head->free = 1;
    kheap_head->next = NULL;
    kheap_tail = kheap_head;
    kheap_total_bytes = pages * PMM_PAGE_SIZE;
    kheap_used_bytes = 0;
}

void *kmalloc(uint32_t size)
{
    if (size == 0) {
        return NULL;
    }

    /* выравнивание запрошенного размера */
    size = kheap_align_up(size);

    for (;;) {
        struct kheap_block *b = kheap_head;

        /* first-fit: первый свободный блок достаточного размера */
        while (b != NULL) {
            if (b->free && b->size >= size) {
                /* разбиваем блок, если остаётся полезный хвост */
                if (b->size >= size + sizeof(struct kheap_block) + KHEAP_ALIGN) {
                    struct kheap_block *rest =
                        (struct kheap_block *)((uint8_t *)b +
                                               sizeof(struct kheap_block) + size);
                    rest->size = b->size - size - sizeof(struct kheap_block);
                    rest->free = 1;
                    rest->next = b->next;
                    b->next = rest;
                }
                b->size = size;
                b->free = 0;
                kheap_used_bytes += b->size;
                return (void *)(b + 1);
            }
            b = b->next;
        }

        /* свободного блока нет: расширяем кучу и пробуем снова */
        if (kheap_grow(size > KHEAP_GROW_MIN ? size : KHEAP_GROW_MIN) != 0) {
            return NULL;
        }
    }
}

void kfree(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    struct kheap_block *b = (struct kheap_block *)ptr - 1;
    if (b->free) {
        return;     /* двойное освобождение */
    }

    b->free = 1;
    kheap_used_bytes -= b->size;

    /* слияние со следующим свободным блоком */
    if (b->next != NULL && b->next->free) {
        b->size += b->next->size + sizeof(struct kheap_block);
        b->next = b->next->next;
    }
    if (b->next == NULL) {
        kheap_tail = b;
    }
}

uint32_t kheap_used(void)
{
    return kheap_used_bytes;
}

uint32_t kheap_total(void)
{
    return kheap_total_bytes;
}
