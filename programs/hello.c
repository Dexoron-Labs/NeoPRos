/*
 * hello.bin — пример программы для NeoPRos.
 * Печатает аргументы командной строки и системную информацию.
 *
 * Сборка: scripts/build_fs.sh (clang + lld, -Ttext=0x400000).
 * Вход: обычная функция; выход — ret (возврат в оболочку).
 */
#include "api/sysapi.h"

void _start(struct neopros_api *api, int argc, char **argv)
{
    api->puts("Hello from NeoPRos!\n");

    api->set_color(10, 0);   /* зелёный */
    api->puts("Arguments: ");
    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            api->puts(" ");
        }
        api->puts(argv[i]);
    }
    api->puts("\n");
    api->set_color(7, 0);    /* белый */

    struct neopros_sysinfo info;
    api->sysinfo(&info);
    api->printf("API version: 0x%x, uptime: %u s\n",
                info.version, info.uptime);
    api->printf("Memory: %u KiB total, %u KiB free\n",
                info.mem_total, info.mem_free);

    struct neopros_time t;
    api->rtc(&t);
    api->printf("Date: %04u-%02u-%02u %02u:%02u:%02u\n",
                t.year, t.month, t.day, t.hour, t.minute, t.second);

    api->puts("Back to the shell.\n");
}
