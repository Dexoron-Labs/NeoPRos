#include "multiboot.h"
#include "gdt.h"
#include "vga.h"
#include "serial.h"

/* Флаги структуры multiboot_info (спецификация Multiboot) */
#define MBI_FLAG_MEMORY      (1u << 0)   /* mem_lower/mem_upper валидны */
#define MBI_FLAG_BOOTLOADER  (1u << 9)   /* boot_loader_name валиден */

/* Магическое число, которое загрузчик передаёт в EAX */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/*
 * Ранняя точка входа ядра на C.
 * Вызывается из boot.asm после установки стека.
 *   magic     — магическое число Multiboot (0x2BADB002),
 *   info_addr — физический адрес структуры multiboot_info.
 */
void kmain(uint32_t magic, uint32_t info_addr)
{
    const struct multiboot_info *info =
        (const struct multiboot_info *)info_addr;

    /* --- инициализация подсистем ядра ---------------------- */
    gdt_init();      /* собственная GDT + перезагрузка сегментов */
    com1_init();     /* COM1 (38400 бод) — дублирование вывода */
    vga_init();      /* VGA text mode 80x25 */

    /* --- заставка ------------------------------------------- */
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    vga_puts("NeoPRos 0.1.0 — 32-bit i386 OS, written by AI\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("================================================\n\n");

    com1_puts("NeoPRos 0.1.0 — 32-bit i386 OS, written by AI\n");

    /* --- проверка магического числа загрузчика -------------- */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        vga_puts("Boot: multiboot (FAILED: bad magic)\n");
        com1_puts("Boot: multiboot (FAILED: bad magic)\n");
        for (;;) {
            __asm__ volatile("hlt");
        }
    }

    vga_puts("Boot: multiboot (OK)\n");
    com1_puts("Boot: multiboot (OK)\n");

    vga_puts("MBI flags: ");
    vga_print_hex32(info->flags);
    vga_puts("\n");
    com1_puts("MBI flags: ");
    com1_print_hex32(info->flags);
    com1_puts("\n");

    /* --- сведения о памяти из multiboot_info ----------------- */
    if (info->flags & MBI_FLAG_MEMORY) {
        vga_puts("Low memory:  ");
        vga_print_dec32(info->mem_lower);
        vga_puts(" KiB\n");
        vga_puts("High memory: ");
        vga_print_dec32(info->mem_upper);
        vga_puts(" KiB\n");

        com1_puts("Low memory:  ");
        com1_print_hex32(info->mem_lower);
        com1_puts("\nHigh memory: ");
        com1_print_hex32(info->mem_upper);
        com1_puts("\n");
    }

    /* --- имя загрузчика -------------------------------------- */
    if (info->flags & MBI_FLAG_BOOTLOADER) {
        const char *name = (const char *)info->boot_loader_name;
        vga_puts("Bootloader: ");
        vga_puts(name);
        vga_puts("\n");
        com1_puts("Bootloader: ");
        com1_puts(name);
        com1_puts("\n");
    }

    vga_puts("Kernel initialized. Halting.\n");
    com1_puts("Kernel initialized. Halting.\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}
