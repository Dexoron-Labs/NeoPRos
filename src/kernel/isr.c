#include "isr.h"
#include "kernel/console.h"
#include "kernel/io.h"

/* Имена исключений i386 (векторы 0..31) */
static const char *const exception_names[] = {
    "Divide-by-zero",        /* 0  */
    "Debug",                 /* 1  */
    "Non-maskable interrupt",/* 2  */
    "Breakpoint",            /* 3  */
    "Overflow",              /* 4  */
    "Bound range exceeded",  /* 5  */
    "Invalid opcode",        /* 6  */
    "Device not available",  /* 7  */
    "Double fault",          /* 8  */
    "Coprocessor segment overrun", /* 9 */
    "Invalid TSS",           /* 10 */
    "Segment not present",   /* 11 */
    "Stack-segment fault",   /* 12 */
    "General protection fault", /* 13 */
    "Page fault",            /* 14 */
    "Reserved",              /* 15 */
    "x87 FPU error",         /* 16 */
    "Alignment check",       /* 17 */
    "Machine check",         /* 18 */
    "SIMD exception",        /* 19 */
    "Virtualization exception", /* 20 */
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved",
};

/* Обработчики IRQ (реализованы в драйверах: pit, kbd и т.д.) */
void irq0_timer(void);
void irq1_keyboard(void);

/* Паника по исключению: печать и остановка */
static void panic(const char *name, struct irq_regs *regs)
{
    console_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    console_puts("\n\nKERNEL PANIC: ");
    console_puts(name);
    console_puts("\nEIP=0x");
    console_print_hex32(regs->eip);
    console_puts(" CS=0x");
    console_print_hex32(regs->cs);
    console_puts(" EAX=0x");
    console_print_hex32(regs->eax);
    console_puts(" EBP=0x");
    console_print_hex32(regs->ebp);
    console_puts("\nSystem halted.\n");

    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

/* Отправка EOI в контроллер 8259 */
static void pic_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

void isr_dispatch(struct irq_regs *regs)
{
    uint32_t vec = regs->vector;

    if (vec < 32) {
        const char *name = (vec < 21) ? exception_names[vec] : "Reserved";
        panic(name, regs);
    }

    if (vec >= 32 && vec <= 47) {
        uint8_t irq = (uint8_t)(vec - 32);
        switch (irq) {
        case 0: irq0_timer(); break;
        case 1: irq1_keyboard(); break;
        default: break;
        }
        pic_eoi(irq);
        return;
    }

    panic("Unhandled interrupt vector", regs);
}
