/*
 * tail.bin — последние 10 строк файла (порт из x16-PRos).
 * Usage: tail <file>
 */
#include "api/sysapi.h"

void _start(struct neopros_api *api, int argc, char **argv)
{
    if (argc < 2) {
        api->puts("Usage: tail <file>\n");
        return;
    }

    static char buf[16384];
    uint32_t size = api->fs_load(argv[1], buf, sizeof(buf));
    if (size == 0) {
        if (api->fs_exists(argv[1])) {
            api->printf("tail: %s: file too big\n", argv[1]);
        } else {
            api->printf("tail: %s: no such file\n", argv[1]);
        }
        return;
    }

    /* считаем строки; ищем начало 10-й с конца */
    uint32_t lines = 0;
    for (uint32_t i = 0; i < size; i++) {
        if (buf[i] == '\n') {
            lines++;
        }
    }
    uint32_t to_skip = (lines > 10) ? lines - 10 : 0;
    uint32_t start = 0;
    for (uint32_t i = 0; i < size && to_skip > 0; i++) {
        if (buf[i] == '\n') {
            start = i + 1;
            to_skip--;
        }
    }

    for (uint32_t i = start; i < size; i++) {
        api->putc(buf[i]);
    }
    if (size > 0 && buf[size - 1] != '\n') {
        api->putc('\n');
    }
}
