/*
 * chars.bin — таблица всех 256 символов (порт из x16-PRos).
 */
#include "api/sysapi.h"

void _start(struct neopros_api *api, int argc, char **argv)
{
    (void)argc;
    (void)argv;

    for (uint32_t i = 0; i < 256; i++) {
        api->printf("%u:%c  ", i, (i >= 0x20 && i <= 0x7E) ? (char)i : '.');
        if (i % 8 == 7) {
            api->putc('\n');
        }
    }
    api->putc('\n');
}
