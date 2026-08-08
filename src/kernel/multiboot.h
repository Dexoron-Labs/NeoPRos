#ifndef NEOPROS_MULTIBOOT_H
#define NEOPROS_MULTIBOOT_H

/* Собственные целочисленные типы: ядро не зависит от libc. */
typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;
typedef int            int32_t;
typedef unsigned long long uint64_t;

/* Универсальный нулевой указатель. */
#define NULL ((void *)0)

/* Магическое число, которое загрузчик кладёт в EAX перед входом в ядро. */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/* Структура multiboot_info, адрес которой загрузчик передаёт в EBX. */
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;   /* объём низкой памяти, КиБ */
    uint32_t mem_upper;   /* объём высокой памяти, КиБ */
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
};

#endif /* NEOPROS_MULTIBOOT_H */
