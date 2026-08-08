/* ============================================================
 *  NeoPRos — загрузочный код ядра (GAS, Intel-синтаксис)
 *  Формат: Multiboot (flat binary), 32-битный protected mode.
 *  Загрузчик (QEMU, GRUB) загружает образ по адресам,
 *  указанным в Multiboot-заголовке, и передаёт управление
 *  на точку входа _start.
 *  Примечание: DCR собирает ассемблер через clang (бэкенд
 *  unix_cc), поэтому используется синтаксис GAS.
 * ============================================================ */

.intel_syntax noprefix

/* --- Константы Multiboot-заголовка --------------------------- */
.equ MB_MAGIC,    0x1BADB002
/* бит 0: модули по 4 КиБ, бит 1: карта памяти,
 * бит 16 (HAS_ADDR): адреса загрузки заданы в заголовке */
.equ MB_FLAGS,    0x00010003

/* --- Точка входа (начало образа) ------------------------------ */
.section .text
.global _start
.extern kmain

_start:
    cli                         /* запрещаем прерывания до настройки IDT */

    /* --- сохраняем аргументы Multiboot: ---------------------- */
    /* eax = магическое число, ebx = адрес multiboot_info.       */
    /* Запоминаем их в edi/esi сразу: дальше boot-код пользуется */
    /* только AL, что испортило бы младший байт EAX.             */
    mov edi, eax
    mov esi, ebx

    /* --- устанавливаем собственный стек ядра ----------------- */
    mov esp, offset stack_top

    /* --- передаём аргументы и входим в ядро на C ------------- */
    push esi                    /* адрес структуры multiboot_info */
    push edi                    /* магическое число 0x2BADB002 */
    call kmain

1:                              /* сюда возвращаемся, если kmain завершится */
    cli
    hlt
    jmp 1b

/* --- Multiboot-заголовок ------------------------------------- */
/* Загрузчик сканирует первые 8 КиБ образа в поисках заголовка.
 * Поля адресов заданы через символы линкера, поэтому они всегда
 * соответствуют реальной раскладке секций. */
.section .multiboot, "a", @progbits
.align 4
.extern _mb_load_end, _mb_bss_end
mb_header:
    .long MB_MAGIC                          /* magic */
    .long MB_FLAGS                          /* flags */
    .long -(MB_MAGIC + MB_FLAGS)            /* checksum */
    .long mb_header                         /* header_addr */
    .long _start                            /* load_addr */
    .long _mb_load_end                      /* load_end_addr */
    .long _mb_bss_end                       /* bss_end_addr */
    .long _start                            /* entry_addr */

/* --- стек ядра ------------------------------------------------ */
/* Стек — обычная (PROGBITS) секция: входит в образ нулями и
 * гарантированно загружается Multiboot-загрузчиком. */
.section .stack, "aw", @progbits
.align 16
stack_bottom:
    .skip 16384
stack_top:
