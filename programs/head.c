/*
 * head.bin — первые 10 строк файла (порт из x16-PRos).
 * Usage: head <file>
 */
#include "api/sysapi.h"

void _start(struct neopros_api *api, int argc, char **argv)
{
    if (argc < 2) {
        api->puts("Usage: head <file>\n");
        return;
    }

    static char buf[16384];
    uint32_t size = api->fs_load(argv[1], buf, sizeof(buf));
    if (size == 0) {
        if (api->fs_exists(argv[1])) {
            api->printf("head: %s: file too big\n", argv[1]);
        } else {
            api->printf("head: %s: no such file\n", argv[1]);
        }
        return;
    }

    uint32_t lines = 0;
    for (uint32_t i = 0; i < size && lines < 10; i++) {
        char c = buf[i];
        api->putc(c);
        if (c == '\n') {
            lines++;
        }
    }
    if (size > 0 && buf[size - 1] != '\n') {
        api->putc('\n');
    }
}
