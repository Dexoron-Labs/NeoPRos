#include "multiboot.h"
#include "gdt.h"
#include "io.h"

/* Вывод одиночного символа в COM1 (диагностика до появления VGA). */
static void com1_putc(char c)
{
    /* ждём, пока передатчик освободится */
    while ((inb(0x3FD) & 0x20) == 0) {
    }
    outb(0x3F8, (uint8_t)c);
}

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

    /* Переходим на собственную GDT и перезагружаем сегменты. */
    gdt_init();
    com1_putc('G');   /* маркер: GDT установлена, сегменты перезагружены */

    for (;;) {
        __asm__ volatile("hlt");
    }
}
