/*
 * grep.bin — поиск текста в файле (порт из x16-PRos).
 * Usage: grep <file> <search_string>
 */
#include "api/sysapi.h"

/* Простой поиск подстроки (нет libc). */
static const char *find_substr(const char *hay, const char *needle)
{
    if (*needle == '\0') {
        return hay;
    }
    for (const char *h = hay; *h; h++) {
        const char *n = needle;
        const char *p = h;
        while (*n && *p && *p == *n) {
            p++;
            n++;
        }
        if (*n == '\0') {
            return h;
        }
    }
    return NULL;
}

void _start(struct neopros_api *api, int argc, char **argv)
{
    if (argc < 3) {
        api->puts("Usage: grep <file> <search_string>\n");
        return;
    }

    static char buf[16384];
    uint32_t size = api->fs_load(argv[1], buf, sizeof(buf));
    if (size == 0) {
        if (api->fs_exists(argv[1])) {
            api->printf("grep: %s: file too big\n", argv[1]);
        } else {
            api->printf("grep: %s: no such file\n", argv[1]);
        }
        return;
    }

    /* идём по строкам: печатаем только совпадающие */
    char line[512];
    uint32_t len = 0;
    uint32_t matches = 0;
    for (uint32_t i = 0; i <= size; i++) {
        if (i < size && buf[i] != '\n' && len < sizeof(line) - 1) {
            line[len++] = buf[i];
            continue;
        }
        line[len] = '\0';
        if (find_substr(line, argv[2]) != NULL) {
            api->puts(line);
            api->putc('\n');
            matches++;
        }
        len = 0;
    }
    api->printf("%u match%s\n", matches, matches == 1 ? "" : "es");
}
