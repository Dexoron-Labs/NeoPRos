/* NeoPRos — скрипт линковки ядра.
   Образ загружается Multiboot-загрузчиком по адресу 1 МиБ (0x00100000). */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* Исполняемый код — должен лежать в начале образа,
       т.к. загрузчик передаёт управление на начало файла. */
    .text ALIGN(4K) :
    {
        *(.text .text.*)
    }

    /* Multiboot-заголовок — обязан находиться в первых 8 КиБ образа. */
    .multiboot ALIGN(4) :
    {
        KEEP(*(.multiboot))
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata .rodata.*)
    }

    .data ALIGN(4K) :
    {
        *(.data .data.*)
    }

    .stack ALIGN(16) :
    {
        *(.stack .stack.*)
    }
    /* конец данных, попадающих в образ */
    _mb_load_end = .;

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss .bss.*)
    }
    /* конец всей памяти ядра (данные + bss) */
    _mb_bss_end = .;
}
