<div align="center">

# NeoPRos

**32-битная операционная система для архитектуры i386, полностью написанная нейросетью**

[![License](https://img.shields.io/badge/license-GPLv3-orange?style=for-the-badge)](LICENSE.TXT)
[![Architecture](https://img.shields.io/badge/arch-i386%20(32%20bit)-blue?style=for-the-badge)]()
[![Language](https://img.shields.io/badge/language-C11-lightgrey?style=for-the-badge)]()
[![Build](https://img.shields.io/badge/build-DCR%20+%20clang-1f425f?style=for-the-badge)]()

---

**NeoPRos** — учебная операционная система для x86, работающая в 32-битном
protected mode. Пишется на чистом C11 с минимальным количеством ассемблера
(только загрузка, точка входа и обработчики прерываний).

</div>

## О проекте

Этот проект — эксперимент по чистому ИИ-кодингу: **весь код, документация и
конфигурация на 100% написаны нейросетью**. Человеческий код в репозитории
отсутствует.

NeoPRos является полным переписыванием проекта **x16-PRos** (16-битная ОС на
NASM для real mode) на современный C11 под архитектуру **i386** в 32-битном
protected mode.

### Ключевые отличия от предшественника

| | x16-PRos (старый) | NeoPRos (новый) |
|---|---|---|
| Режим | 16-битный real mode | 32-битный protected mode |
| Язык | NASM (100%) | C11 + минимум NASM (boot/entry) |
| Загрузка | собственный bootloader | Multiboot-совместимое ядро |
| Компилятор | NASM | clang (i386-none-elf) |
| Сборка | bash-скрипты | **DCR** (dcr.toml) |
| Целевая платформа | x86 (16 бит) | i386 (32 бита) |

## Текущие возможности

- Multiboot-совместимая загрузка (загрузчик: QEMU, GRUB и др.)
- Точка входа ядра в 32-битном protected mode
- Минимальная GDT с плоской моделью памяти (4 GiB)
- Вывод в VGA text mode (80x25, 0xB8000)
- Вывод в COM1 (serial) для отладки
- Чёткое разделение: boot на ассемблере, ядро на C

## Структура проекта

```
NeoPRos/
├── dcr.toml              # конфигурация сборки DCR
├── linker.ld-подобный:    # смотри src/linker.ld
├── src/
│   ├── boot/
│   │   └── boot.asm      # multiboot-заголовок + точка входа (_start)
│   ├── kernel/
│   │   ├── kmain.c       # ранняя точка входа ядра на C
│   │   ├── vga.c/.h      # вывод в VGA text mode
│   │   ├── serial.c/.h   # вывод в COM1
│   │   ├── gdt.c/.h      # минимальная GDT
│   │   └── multiboot.h   # структуры Multiboot info
│   └── linker.ld         # скрипт линковки (ядро по адресу 0x00100000)
├── scripts/
│   └── run.sh            # запуск в QEMU
├── LICENSE.TXT           # GPL-3.0
└── README.md
```

## Сборка

Требуется: **DCR 0.8.x**, **clang**, **NASM**, GNU ld или lld.

```bash
dcr build                 # отладочная сборка
dcr build --release       # релизная сборка
```

Артефакт: `target/i386-none-elf/debug/neopros` — Multiboot-совместимый ELF.

## Запуск в QEMU

```bash
scripts/run.sh            # qemu-system-i386 -kernel <образ>
```

Или вручную:

```bash
qemu-system-i386 -kernel target/i386-none-elf/debug/neopros -serial stdio
```

На экране появится приветствие NeoPRos, вывод VGA продублируется в COM1.

## Планы

- [ ] IDT и обработчики исключений
- [ ] Управление памятью (paging, PFA)
- [ ] Клавиатура (PS/2)
- [ ] Таймер (PIT) и планировщик
- [ ] Пользовательский режим (ring 3)

## Лицензия

[GNU GPL v3](LICENSE.TXT)

---

> Проект является экспериментом по ИИ-кодингу. Весь код сгенерирован
> нейросетью и публикуется в образовательных целях.
