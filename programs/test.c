/*
 * test.bin — проверка файлового API из программы:
 * текущий каталог, список корня, чтение README.TXT.
 */
#include "api/sysapi.h"

void _start(struct neopros_api *api, int argc, char **argv)
{
    (void)argc;
    (void)argv;

    char buf[512];

    api->puts("FS API test\n");

    api->fs_getcwd(buf, sizeof(buf));
    api->printf("cwd: %s\n", buf);

    uint32_t n = api->fs_list(NULL, buf, sizeof(buf));
    if (n == 0xFFFFFFFFu) {
        api->puts("list: error\n");
        return;
    }
    api->printf("root (%u entries):\n", n);
    api->puts(buf);

    if (api->fs_exists("README.TXT")) {
        api->puts("README.TXT exists\n");
        uint32_t sz = api->fs_load("README.TXT", buf, sizeof(buf));
        api->printf("README.TXT (%u bytes):\n", sz);
        buf[sz] = '\0';
        api->puts(buf);
    } else {
        api->puts("README.TXT not found\n");
    }

    api->puts("FS API test done.\n");

    api->exit(0); /* проверка sysapi.exit(): return через longjmp */
}
