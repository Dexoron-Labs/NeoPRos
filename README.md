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
(только загрузка и точка входа).

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
| Язык | NASM (100%) | C11 + минимум ассемблера (boot/entry) |
| Загрузка | собственный bootloader | Multiboot-совместимое ядро |
| Компилятор | NASM | clang (i386-none-elf) |
| Сборка | bash-скрипты | **DCR** (dcr.toml) |
| Целевая платформа | x86 (16 бит) | i386 (32 бита) |

## Текущие возможности

- Multiboot-совместимая загрузка (загрузчики: QEMU, GRUB и др.)
- Точка входа ядра в 32-битном protected mode
- Минимальная GDT с плоской моделью памяти (0..4 ГБ)
- Вывод в VGA text mode (80x25, 0xB8000) с цветом и прокруткой
- Вывод в COM1 (serial, 38400 бод) — дублирование консоли для отладки
- Диагностика Multiboot-информации: память, имя загрузчика
- Чёткое разделение: boot на ассемблере, ядро на C

## Структура проекта

```
NeoPRos/
├── dcr.toml              # конфигурация сборки DCR
├── src/
│   ├── boot/
│   │   └── boot.S        # multiboot-заголовок + точка входа (_start)
│   ├── kernel/
│   │   ├── kmain.c       # ранняя точка входа ядра на C
│   │   ├── gdt.c/.h      # минимальная GDT (плоские сегменты)
│   │   ├── vga.c/.h      # вывод в VGA text mode
│   │   ├── serial.c/.h   # вывод в COM1
│   │   ├── io.h          # операции ввода-вывода (inb/outb)
│   │   └── multiboot.h   # структуры Multiboot info
│   └── linker.ld         # скрипт линковки (ядро по адресу 0x00100000)
├── scripts/
│   └── run.sh            # запуск в QEMU
├── LICENSE.TXT           # GPL-3.0
└── README.md
```

## Как это работает

1. **Загрузка** — QEMU (или GRUB) находит Multiboot-заголовок в первых
   8 КиБ образа, загружает ядро по адресу 1 МиБ и входит в 32-битный
   protected mode. В EAX передаётся магическое число `0x2BADB002`,
   в EBX — адрес структуры `multiboot_info`.
2. **boot.S** — сохраняет аргументы загрузчика, инициализирует COM1,
   устанавливает собственный стек и вызывает `kmain` на C.
3. **kmain** — инициализирует GDT (собственные плоские сегменты кода
   и данных 0..4 ГБ), COM1, VGA, выводит приветствие и диагностику.

Ассемблер собран синтаксисом GAS (Intel-нотация): DCR обрабатывает
`.S`-файлы через clang, а `.c` — через бэкенд C. Оба языка описаны
в `dcr.toml` (`language = "c,asm"`).

## Сборка

Требуется: **DCR 0.8.x**, **clang**, GNU ld/lld (binutils).

```bash
dcr build                 # отладочная сборка
dcr build --release       # релизная сборка
dcr build --clean         # полная пересборка
```

Артефакт: `target/i386-none-elf/debug/neopros.bin` — Multiboot-совместимый
плоский образ ядра.

## Запуск в QEMU

```bash
scripts/run.sh            # debug
scripts/run.sh --release  # релизная сборка
```

Или вручную:

```bash
qemu-system-i386 -kernel target/i386-none-elf/debug/neopros.bin -serial stdio
```

Примерный вывод на экране и в COM1:

```
NeoPRos 0.1.0 — 32-bit i386 OS, written by AI
================================================

Boot: multiboot (OK)
MBI flags: 0x0000024f
Low memory:  639 KiB
High memory: 129920 KiB
Bootloader: qemu
Kernel initialized. Halting.
```

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
