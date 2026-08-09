/*
 * fetch.bin — системная информация в стиле fetch (порт из x16-PRos).
 * Печатает ASCII-арт и основные параметры системы.
 */
#include "api/sysapi.h"

void _start(struct neopros_api *api, int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct neopros_sysinfo info;
    api->sysinfo(&info);
    struct neopros_time t;
    api->rtc(&t);

    api->puts("      _   ____   _____          _       \n");
    api->puts("     | \\ | |  _ \\|  __ \\        | |      \n");
    api->puts("     |  \\| | |_) | |__) |___  __| | ___  \n");
    api->puts("     | . ` |  _ <|  ___/ _ \\/ _` |/ _ \\ \n");
    api->puts("     | |\\  | |_) | |  |  __/ (_| | (_) |\n");
    api->puts("     |_| \\_|____/|_|   \\___|\\__,_|\\___/ \n");
    api->puts("\n");

    api->set_color(6, 0);
    api->puts("       NeoPRos 0.1.0 (i386)\n");
    api->set_color(7, 0);

    api->printf("       uptime:   %u s\n", info.uptime);
    api->printf("       memory:   %u KiB total, %u KiB free\n",
                info.mem_total, info.mem_free);
    api->printf("       date:     %04u-%02u-%02u\n",
                t.year, t.month, t.day);
    api->printf("       time:     %02u:%02u:%02u\n",
                t.hour, t.minute, t.second);
    api->printf("       api:      0x%x (version %u)\n",
                (uint32_t)api, info.version);
    api->puts("\n");
}
