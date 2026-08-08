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
- IDT на 256 векторов: исключения с паникой, аппаратные прерывания
- PIC 8259: remap на 0x20/0x28, маскирование отдельных IRQ
- Пит-таймер (100 Гц) со счётчиком тиков
- Драйвер клавиатуры PS/2: набор сканкодов set 1, Shift, буфер;
  расширенные клавиши (стрелки, Home/End, Delete) — в ANSI-последовательности
- Часы реального времени (CMOS RTC): дата и время (BCD, 24h)
- Физический аллокатор страниц (bitmap по Multiboot mmap)
- Куча ядра: `kmalloc`/`kfree` (first-fit, рост на лету)
- Readline-терминал: редактирование строки, история команд
- Командная оболочка **prosh** (заменяемая): help, ver, info, cls, echo,
  date, time, calc, memory, uptime, reboot, shutdown, exit
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
│   │   ├── boot.S        # multiboot-заголовок + точка входа (_start)
│   │   └── isr_stubs.S   # 256 stub'ов IDT + общий обработчик (сгенерировано)
│   ├── kernel/
│   │   ├── kmain.c       # ранняя точка входа ядра на C
│   │   ├── gdt.c/.h      # минимальная GDT (плоские сегменты)
│   │   ├── idt.c/.h      # IDT, PIC 8259 (remap, маски IRQ)
│   │   ├── isr.c/.h      # диспетчер прерываний, исключения, паника
│   │   ├── pit.c/.h      # программируемый таймер (100 Гц)
│   │   ├── kbd.c/.h      # клавиатура PS/2 (set 1, буфер, ANSI)
│   │   ├── rtc.c/.h      # часы реального времени (CMOS)
│   │   ├── pmm.c/.h      # физический менеджер страниц (bitmap)
│   │   ├── kheap.c/.h    # куча ядра (kmalloc/kfree)
│   │   ├── readline.c/.h # построчный ввод с редактированием
│   │   ├── shell.c/.h    # командная оболочка prosh
│   │   ├── commands.c/.h # встроенные команды shell
│   │   ├── console.c/.h  # единая консоль: VGA + COM1, kprintf
│   │   ├── vga.c/.h      # вывод в VGA text mode
│   │   ├── serial.c/.h   # вывод в COM1
│   │   ├── string.c/.h   # утилиты работы со строками и памятью
│   │   ├── io.h          # операции ввода-вывода (inb/outb/outw)
│   │   └── multiboot.h   # структуры Multiboot info
│   └── linker.ld         # скрипт линковки (ядро по адресу 0x00100000)
├── LICENSE.TXT           # GPL-3.0
└── README.md
```

## Как это работает

1. **Загрузка** — QEMU (или GRUB) находит Multiboot-заголовок в первых
   8 КиБ образа, загружает ядро по адресу 1 МиБ и входит в 32-битный
   protected mode. В EAX передаётся магическое число `0x2BADB002`,
   в EBX — адрес структуры `multiboot_info`.
2. **boot.S** — сохраняет аргументы загрузчика, устанавливает
   собственный стек и вызывает `kmain` на C.
3. **kmain** — инициализирует GDT, консоль (VGA + COM1), выводит
   приветствие и диагностику, затем инициализирует прерывания
   (IDT, PIC, PIT), клавиатуру и RTC.
4. **Память** — `pmm_init` строит битовую карту физических страниц по
   Multiboot-mmap (QEMU отдаёт 64-битные записи), `kheap_init` выделяет
   стартовую кучу 256 КиБ под `kmalloc`/`kfree`.
5. **Прерывания** — каждый вектор IDT имеет свой stub, который
   сохраняет контекст (GAS-синтаксис, 32-битные push сегментов) и
   передаёт управление в общий диспетчер `isr_dispatch` на C.
6. **Оболочка** — `shell_main` запускает цикл: readline печатает
   приглашение `prosh> ` и принимает строку с редактированием
   (стрелки, Home/End, Delete, история по Up/Down), затем
   `shell_run_line` разбирает строку (кавычки и экранирование) и
   выполняет зарегистрированную команду. Оболочка заменяема: свой
   цикл можно построить на `readline()` + `shell_run_line()`.

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
dcr run                   # debug
dcr run --release         # релизная сборка
```

Запуск описан в секции `[run]` файла `dcr.toml` (подстановка `{profile}`
происходит автоматически). Или вручную:

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
Interrupts: OK
RTC: 2026-08-08 15:02:55
Memory: 129920 KiB total, 129600 KiB free
NeoPRos shell (prosh) - type 'help' for commands.
prosh> help
Available commands:
  help        show this help
  ver         print OS version
  ...
prosh> calc 2+3*4
14
```

## Планы

- [x] IDT и обработчики исключений
- [x] Клавиатура (PS/2)
- [x] Таймер (PIT)
- [x] Часы реального времени (RTC)
- [x] Управление памятью (PFA, куча ядра)
- [x] Терминал и команды (prosh)
- [ ] Файловая система (FAT12) и ATA
- [ ] Графика (VGA mode 13h) и звук
- [ ] Пользовательский режим (ring 3)

## Лицензия

[GNU GPL v3](LICENSE.TXT)

---

> Проект является экспериментом по ИИ-кодингу. Весь код сгенерирован
> нейросетью и публикуется в образовательных целях.
