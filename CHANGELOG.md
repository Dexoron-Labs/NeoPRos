# Журнал изменений

История изменений **NeoPRos** (переписывание x16-PRos). Все изменения —
по 2026-08-08, ветка `rewrite/c-i386`.

## v0.1.0 — начальная разработка (2026-08-08)

### Фаза 2: память и оболочка prosh

- Физический менеджер страниц `pmm`: битовая карта по Multiboot-mmap,
  выделение/освобождение непрерывных страниц (учитываются 64-битные
  записи mmap от QEMU 11) ([c67debd]).
- Куча ядра `kheap`: `kmalloc`/`kfree` (first-fit) с автоматическим
  ростом на новые страницы ([c67debd]).
- Readline-терминал: редактирование строки (стрелки, Home/End, Delete,
  Insert, Backspace), история команд (кольцо 16 строк), ввод через
  ANSI-последовательности ([c67debd]).
- Оболочка **prosh**: заменяемые `shell_name`/`shell_prompt`, таблица
  команд через `shell_register`, токенизация с кавычками и
  экранированием ([c67debd]).
- Команды: `help`, `ver`, `info`, `cls`/`clear`, `echo`, `date`, `time`,
  `calc` (рекурсивный спуск: `+ - * / %`, скобки, hex `0x`), `memory`,
  `uptime`, `reboot`, `shutdown`, `exit` ([c67debd]).
- Клавиатура: расширенные клавиши переводятся в ANSI-ESC
  (`\x1b[A` и т.д.), релизы корректно игнорируются ([c67debd]).
- RTC: исправлена семантика бита DM в статусе B (1 = двоичный режим,
  0 = BCD — для BCD бит сбрасывается), `rtc_init()` вызывается до
  первого чтения; двойное чтение регистров против «мусорных» значений
  ([c67debd]).
- `kprintf`: поддержка нулевой ширины `%0Nu` (исправлен вывод
  `date`/`time`/`uptime`) ([c67debd]).
- Readline: аппаратный VGA-курсор следует за позицией редактирования
  при навигации и вставке ([d8c62d7]).
- Доработан README под Фазу 2 ([c67debd]).

### Фаза 1: прерывания и драйверы

- IDT на 256 векторов с индивидуальными asm-стабами, диспетчер
  исключений с паникой и обработчики прерываний ([151f534]).
- PIC 8259: remap на 0x20/0x28, маскирование IRQ ([151f534]).
- PIT-таймер 100 Гц со счётчиком тиков ([151f534]).
- Клавиатура PS/2: set 1, Shift (принимаются релизы в стиле set 2,
  как шлёт QEMU), кольцевой буфер ([151f534]).
- Часы реального времени (CMOS RTC): чтение даты/времени, BCD и
  12/24-часовой форматы ([151f534]).

### Базовое ядро

- Каркас проекта и конфигурация DCR (clang, C11, flat-bin,
  i386-none-elf, freestanding) ([340bd89]).
- Multiboot-загрузка и точка входа ядра i386 (boot.S: заголовок первой
  секцией, стек в BSS) ([4a5a68d]).
- Минимальная GDT с плоскими сегментами ([da633ee]).
- Единая консоль: VGA text mode (80x25, цвета, прокрутка) + COM1
  (serial) с дублированием вывода; `kprintf` (`%s %c %d %u %x %%`)
  ([a9eef3d]).
- `memmove`, `outw`, `NULL`/`int32_t`/`uint64_t` в `multiboot.h`
  (ядро без libc) ([c67debd]).

[c67debd]: https://github.com/Dexoron-Labs/NeoPRos/commit/c67debd
[d8c62d7]: https://github.com/Dexoron-Labs/NeoPRos/commit/d8c62d7
[151f534]: https://github.com/Dexoron-Labs/NeoPRos/commit/151f534
[340bd89]: https://github.com/Dexoron-Labs/NeoPRos/commit/340bd89
[4a5a68d]: https://github.com/Dexoron-Labs/NeoPRos/commit/4a5a68d
[da633ee]: https://github.com/Dexoron-Labs/NeoPRos/commit/da633ee
[a9eef3d]: https://github.com/Dexoron-Labs/NeoPRos/commit/a9eef3d
