#include "multiboot.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "kbd.h"
#include "rtc.h"
#include "console.h"

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
 *
 * Все строки — только ASCII: выводятся в VGA и COM1 одинаково.
 */
void kmain(uint32_t magic, uint32_t info_addr)
{
    const struct multiboot_info *info =
        (const struct multiboot_info *)info_addr;

    /* --- инициализация подсистем ядра ---------------------- */
    gdt_init();        /* собственная GDT + перезагрузка сегментов */
    console_init();    /* VGA text mode + COM1 (единая консоль) */

    /* --- заставка ------------------------------------------- */
    console_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    console_puts("NeoPRos 0.1.0 - 32-bit i386 OS, written by AI\n");
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    console_puts("================================================\n\n");

    /* --- проверка магического числа загрузчика -------------- */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        console_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        console_puts("Boot: multiboot (FAILED: bad magic)\n");
        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }

    console_puts("Boot: multiboot (OK)\n");

    console_puts("MBI flags: ");
    console_print_hex32(info->flags);
    console_puts("\n");

    /* --- сведения о памяти из multiboot_info ----------------- */
    if (info->flags & MBI_FLAG_MEMORY) {
        console_puts("Low memory:  ");
        console_print_dec32(info->mem_lower);
        console_puts(" KiB\n");
        console_puts("High memory: ");
        console_print_dec32(info->mem_upper);
        console_puts(" KiB\n");
    }

    /* --- имя загрузчика -------------------------------------- */
    if (info->flags & MBI_FLAG_BOOTLOADER) {
        const char *name = (const char *)info->boot_loader_name;
        console_puts("Bootloader: ");
        console_puts(name);
        console_puts("\n");
    }

    /* --- прерывания и аппаратные драйверы -------------------- */
    idt_init();        /* IDT на основе asm-stub'ов */
    pic_remap();       /* PIC: remap на 0x20/0x28 */
    pit_init();        /* PIT: 100 Гц */
    kbd_init();        /* PS/2 клавиатура */

    pic_enable_irq(0); /* таймер */
    pic_enable_irq(1); /* клавиатура */

    console_puts("Interrupts: OK\n");

    __asm__ volatile("sti");

    /* --- проверка RTC (часы реального времени) ---------------- */
    {
        struct rtc_time t;
        rtc_get_time(&t);
        console_puts("RTC: ");
        console_print_dec32(t.year);
        console_putc('-');
        console_print_dec32(t.month);
        console_putc('-');
        console_print_dec32(t.day);
        console_putc(' ');
        console_print_dec32(t.hour);
        console_putc(':');
        console_print_dec32(t.minute);
        console_putc(':');
        console_print_dec32(t.second);
        console_puts("\n");
    }

    /* --- тестовый цикл: тики таймера и клавиатура ------------ */
    uint32_t last_print = 0;

    for (;;) {
        if (pit_ticks() - last_print >= PIT_HZ) {
            last_print = pit_ticks();
            console_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
            console_puts("tick ");
            console_print_dec32(pit_ticks());
            console_puts("\n");
            console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }

        int c = kbd_getc();
        if (c != -1) {
            console_puts("key: '");
            console_putc((char)c);
            console_puts("'\n");
        }

        __asm__ volatile("hlt");
    }
}
