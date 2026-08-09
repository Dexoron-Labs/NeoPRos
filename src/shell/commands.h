#ifndef NEOPROS_COMMANDS_H
#define NEOPROS_COMMANDS_H

/*
 * Регистрация всех встроенных команд в глобальной таблице shell.
 * Модуль команд — тоже "плагин": прочие подсистемы (файловая
 * система, графика) будут добавлять свои команды тем же способом.
 */
void commands_init(void);

#endif /* NEOPROS_COMMANDS_H */
