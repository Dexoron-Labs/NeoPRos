#include "kernel/multiboot.h"
#include "gdt.h"
#include "idt.h"
#include "kernel/pit.h"
#include "kernel/kbd.h"
#include "kernel/rtc.h"
#include "kernel/console.h"
#include "mm/pmm.h"
#include "mm/kheap.h"
#include "fs/fs.h"
#include "loader/loader.h"
#include "api/sysapi.h"
#include "shell/shell.h"
#include "shell/commands.h"

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
 * Загрузочные сообщения — в формате x16-PRos: [ OKAY ]/[ WARN ].
 */

/* --- загрузочный лог в стиле x16-PRos (log.asm) ------------------ */

static void log_okay(const char *msg)
{
    console_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    console_puts("[ OKAY ]  ");
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    console_puts(msg);
    console_puts("\n");
}

static void log_warn(const char *msg)
{
    console_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    console_puts("[ WARN ]  ");
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    console_puts(msg);
    console_puts("\n");
}

static void log_error(const char *msg)
{
    console_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    console_puts("[ ERROR ] ");
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    console_puts(msg);
    console_puts("\n");
}

void kmain(uint32_t magic, uint32_t info_addr)
{
    const struct multiboot_info *info =
        (const struct multiboot_info *)info_addr;

    /* --- инициализация подсистем ядра ---------------------- */
    gdt_init();        /* собственная GDT + перезагрузка сегментов */
    console_init();    /* VGA text mode + COM1 (единая консоль) */
    log_okay("Segment initialization");

    /* --- проверка магического числа загрузчика -------------- */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        log_error("Boot: multiboot (FAILED: bad magic)");
        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }

    /* --- прерывания и аппаратные драйверы -------------------- */
    idt_init();        /* IDT на основе asm-stub'ов */
    pic_remap();       /* PIC: remap на 0x20/0x28 */
    pit_init();        /* PIT: 100 Гц */
    kbd_init();        /* PS/2 клавиатура */

    pic_enable_irq(0); /* таймер */
    pic_enable_irq(1); /* клавиатура */

    log_okay("Timer initialization");

    __asm__ volatile("sti");

    /* --- проверка RTC (часы реального времени) ---------------- */
    rtc_init();                      /* RTC: BCD + 24h */

    {
        struct rtc_time t;
        rtc_get_time(&t);
        kprintf("RTC: %04u-%02u-%02u %02u:%02u:%02u\n",
                t.year, t.month, t.day, t.hour, t.minute, t.second);
    }

    /* --- менеджеры памяти ------------------------------------- */
    /* ФС инициализируется до pmm_init: битовая карта pmm размещается
     * сразу после ядра и перекрывает список модулей Multiboot,
     * так что после pmm_init модули уже не найти. */
    fs_init(info);                  /* FAT12: RAM-диск (multiboot-модуль) */
    pmm_init(info);                 /* физическая память (bitmap) */
    loader_init();                  /* резерв зоны программ .BIN */
    pmm_reserve_range(fs_image_base(), fs_image_size());  /* RAM-диск */
    kheap_init(256u * 1024u);       /* куча ядра: 256 КиБ стартово */

    log_okay("Memory allocator");

    /* --- файловая система (RAM-диск из multiboot-модуля) ------ */
    if (fs_available()) {
        log_okay("File System API (FAT12 ramdisk)");
    } else {
        log_warn("File System API (no RAM disk)");
    }

    /* --- системный API ------------------------------------------ */
    sysapi_init(info_addr);
    log_okay("API initialization");

    /* --- командная оболочка prosh ----------------------------- */
    commands_init();
    log_okay("Shell initialization");

    if (shell_main() == SHELL_EXIT) {
        console_puts("\nSystem halted.\n");
        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }
}
