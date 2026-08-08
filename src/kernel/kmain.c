#include "multiboot.h"

/*
 * Ранняя точка входа ядра на C.
 * Вызывается из boot.asm после установки стека.
 *   magic     — магическое число Multiboot (0x2BADB002),
 *   info_addr — физический адрес структуры multiboot_info.
 */
void kmain(uint32_t magic, uint32_t info_addr)
{
    (void)magic;
    (void)info_addr;

    for (;;) {
        __asm__ volatile("hlt");
    }
}
