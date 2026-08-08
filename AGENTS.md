# AGENTS.md

## Проект

NeoPRos — 32-битная ОС для i386, полное переписывание x16-PRos (16-бит,
NASM) на C11. Рабочая ветка: `rewrite/c-i386` (коммиты только отсюда,
пушить в неё). Весь код пишется ИИ, правила — в README.md.

## Сборка и запуск

- Сборка: `dcr build --force`. **Обязательно** перед ней:
  `rm -rf target/i386-none-elf/debug` — иначе DCR не замечает изменений
  и «собирает» без перекомпиляции.
- Запуск: `dcr run` (qemu-system-i386 + `-serial stdio`), профиль в
  `target/i386-none-elf/debug/neopros.bin`. Тестов нет, проверка — только
  в QEMU.
- Правки в src/boot (`.S`, GAS, Intel-синтаксис) пересобираются clang-ом.

## Тестирование в QEMU

- Убивать QEMU только так: `pkill -f 'qemu-system-[i]386'` — приём
  `[i]386` обязателен, иначе pkill убивает собственную shell-команду
  (она содержит «qemu-system» в командной строке).
- Запуск без дисплея с монитором:
  `qemu-system-i386 -kernel .../neopros.bin -serial stdio -display none
  -monitor unix:/tmp/mon.sock,server,nowait`
- `sendkey` QEMU: события идут очередью с задержкой 100 мс на клавишу —
  слать по одной клавише с паузами, иначе «фантомные» повторные символы.
- Имена клавиш QEMU: `spc` (не `space`), `ret`, `minus`, `slash`,
  `equal`, `asterisk` (это keypad-*). Заглавные буквы как имя клавиши
  (`sendkey X`) НЕ включают shift — использовать `shift-a` и т.п.
  Шифт обрабатывается ядром корректно (`shift-a` → «A»).
- Проверка позиции VGA-курсора: `screendump` в мониторе + анализ PPM
  (курсор — горизонтальная линия внизу клетки). Мигание курсора даёт
  пустые кадры — снимать несколько кадров.

## Архитектура

- Вход: `src/boot/boot.S` (multiboot-заголовок должен быть первой
  секцией, load_addr кратен 4К, `_mb_bss_end` после `.stack`) →
  `src/kernel/kmain.c` (GDT → консоль → IDT/PIC/PIT → kbd → rtc_init →
  pmm_init → kheap_init → commands_init → shell_main).
- Стек вызовов сборки: каждый вектор IDT — свой stub в
  `src/boot/isr_stubs.S` → `isr_dispatch` в `src/kernel/isr.c`.
- Оболочка заменяемая: `shell_name`/`shell_prompt` — внешние
  `const char *`, команды через `shell_register`, цикл = `readline()` +
  `shell_run_line()`. Новая оболочка = свой цикл поверх этих API.
- Расширенные клавиши (стрелки, Home/End/Delete) в `kbd.c` переводятся
  в ANSI-ESC-последовательности (`\x1b[A` и т.д.) — readline и всё
  остальное работают с единым потоком символов.

## Подводные камни

- Без libc: `NULL`, `int32_t`, `uint64_t` объявлены в `multiboot.h`.
  QEMU 11 отдаёт mmap-записи с 64-битными addr/len
  (`struct multiboot_mmap_entry`), порядок полей: size, addr, len, type.
- RTC: бит DM (0x04) в status B работает «наоборот» — 1 = двоичный
  режим, 0 = BCD. Для BCD его нужно СБРАСЫВАТЬ (rtc_init). QEMU RTC
  показывает UTC. `rtc_init()` должен вызываться до первого
  `rtc_get_time()`.
- `kprintf` (console.c) понимает только `%s %c %d %u %x %%` и нулевую
  ширину `%0Nu`; другие спецификаторы печатаются как есть.
- Тексты ядра — только ASCII: консоль дублирует вывод в VGA и COM1
  одинаковыми байтами, UTF-8 не поддерживается. Комментарии и README —
  на русском, коммиты — на английском (imperative mood).
- `outb(0x64, 0xFE)` = reboot, `outw(0x604, 0x2000)` = ACPI shutdown
  (см. commands.c).
