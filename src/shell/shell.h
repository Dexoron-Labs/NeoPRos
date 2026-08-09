#ifndef NEOPROS_SHELL_H
#define NEOPROS_SHELL_H

#include "kernel/multiboot.h"

/* Максимум зарегистрированных команд и аргументов в строке. */
#define SHELL_MAX_COMMANDS 32
#define SHELL_MAX_ARGS 16

/*
 * Команда ядра. Оболочка и любые потребители (будущий sh)
 * исполняют команды через единую точку входа — shell_exec.
 *
 * Возвращаемое значение: 0 — норма, SHELL_EXIT — завершить оболочку.
 */
#define SHELL_EXIT 1

struct shell_command {
    const char *name;              /* имя команды (ASCII, lowercase) */
    const char *help;              /* краткая справка для `help` */
    int (*handler)(int argc, char **argv);
};

/*
 * Регистрация команды в глобальной таблице. Вызывается при
 * инициализации модулей команд.
 */
void shell_register(const struct shell_command *cmd);

/* Поиск команды по имени (без учёта регистра). */
const struct shell_command *shell_find(const char *name);

/* Перечисление команд таблицы: index 0.., NULL по исчерпанию. */
const struct shell_command *shell_iter(uint32_t index);

/*
 * Исполнение строки: разбор на аргументы и вызов shell_exec.
 * Публичный API — будущий sh сможет исполнять команды ядра.
 */
int shell_run_line(const char *line);

/*
 * Диспетчер: поиск команды по argv[0] и вызов обработчика.
 * Если команда не найдена, вызывается fallback (если установлен):
 * возврат 0 — команда обработана, -1 — не распознана.
 * Возвращает код обработчика (SHELL_EXIT — завершение оболочки).
 */
int shell_exec(int argc, char **argv);

/* Установить обработчик неизвестных команд (запуск .BIN по имени
 * в стиле x16-PRos). */
void shell_set_fallback(int (*fn)(int argc, char **argv));

/*
 * Главный цикл оболочки prosh: приглашение → readline →
 * shell_run_line. Возвращает SHELL_EXIT при команде `exit`.
 *
 * Точка подстановки: чтобы заменить prosh на другой интерпретатор
 * (например, sh), достаточно не вызывать shell_main(), а написать
 * свой цикл поверх readline()/shell_run_line().
 */
int shell_main(void);

/* Имя и приглашение оболочки — единственное место, где они задаются. */
extern const char *shell_name;
extern const char *shell_prompt;

#endif /* NEOPROS_SHELL_H */
