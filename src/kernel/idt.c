#include "idt.h"
#include "kernel/io.h"

/* Адреса stub'ов сгенерированы в src/boot/isr_stubs.S */
extern uint32_t isr_stub_table[IDT_ENTRIES];

/* Шлюз IDT (32-битный interrupt gate) */
struct idt_gate {
    uint16_t offset_low;    /* младшие 16 бит адреса обработчика */
    uint16_t selector;      /* селектор кода ядра */
    uint8_t  zero;          /* всегда 0 */
    uint8_t  type_attr;     /* тип и атрибуты */
    uint16_t offset_high;   /* старшие 16 бит адреса обработчика */
} __attribute__((packed));

/* Псевдо-дескриптор для инструкции lidt */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* Таблица шлюзов */
static struct idt_gate idt[IDT_ENTRIES];

/* Атрибуты: present, DPL=0, 32-битный interrupt gate (0xE) */
#define IDT_GATE_PRESENT 0x80
#define IDT_GATE_INTERRUPT32 0x0E

static void idt_set_gate(int n, uint32_t handler)
{
    idt[n].offset_low = (uint16_t)(handler & 0xFFFF);
    idt[n].selector = 0x08;              /* сегмент кода ядра */
    idt[n].zero = 0;
    idt[n].type_attr = IDT_GATE_PRESENT | IDT_GATE_INTERRUPT32;
    idt[n].offset_high = (uint16_t)(handler >> 16);
}

void pic_remap(void)
{
    /* ICW1: инициализация, каскад, по 4 байта */
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    /* ICW2: базовые векторы 0x20 (master) и 0x28 (slave) */
    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();

    /* ICW3: каскад — master знает slave на IRQ2, slave — свой каскад */
    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();

    /* ICW4: 8086 mode, автоматический EOI выключен */
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    /* маска: гасим всё; драйверы разрешат нужные IRQ */
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void pic_enable_irq(uint8_t irq)
{
    uint16_t port = (irq < 8) ? 0x21 : 0xA1;
    uint8_t mask = inb(port);
    mask &= (uint8_t)~(1u << (irq & 7));
    outb(port, mask);
}

void pic_disable_irq(uint8_t irq)
{
    uint16_t port = (irq < 8) ? 0x21 : 0xA1;
    uint8_t mask = inb(port);
    mask |= (uint8_t)(1u << (irq & 7));
    outb(port, mask);
}

void idt_init(void)
{
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, isr_stub_table[i]);
    }

    struct idt_ptr ptr;
    ptr.limit = sizeof(idt) - 1;
    ptr.base = (uint32_t)idt;

    __asm__ volatile("lidt %0" : : "m"(ptr) : "memory");
}
