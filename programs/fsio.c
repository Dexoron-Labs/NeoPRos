/*
 * fsio.bin — проверка записи в ФС (порт FAT12-write из x16-PRos):
 * создание, перезапись, чтение, каталоги, переименование, удаление.
 */
#include "api/sysapi.h"
#include <stdint.h>

void _start(struct neopros_api *api, int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char buf[512];
    uint32_t before = api->fs_free_space();

    /* 1. создание файла */
    if (!api->fs_write("A.TXT", "hello fs\n", 9)) {
        api->puts("FAIL: create A.TXT\n");
        return;
    }
    /* 2. чтение */
    uint32_t n = api->fs_load("A.TXT", buf, sizeof(buf));
    if (n != 9) {
        api->puts("FAIL: read size\n");
        return;
    }
    buf[n] = '\0';
    api->printf("A.TXT: %u bytes, content: %s", n, buf);

    /* 3. перезапись */
    if (!api->fs_write("A.TXT", "rewritten!", 10)) {
        api->puts("FAIL: rewrite\n");
        return;
    }
    n = api->fs_load("A.TXT", buf, sizeof(buf));
    buf[n] = '\0';
    api->printf("rewritten: %s\n", buf);

    /* 4. каталог */
    if (!api->fs_mkdir("DIR1")) {
        api->puts("FAIL: mkdir\n");
        return;
    }
    if (!api->fs_is_dir("DIR1")) {
        api->puts("FAIL: is_dir\n");
        return;
    }
    if (!api->fs_write("DIR1/INNER.TXT", "nested\n", 7)) {
        api->puts("FAIL: write nested\n");
        return;
    }
    n = api->fs_load("DIR1/INNER.TXT", buf, sizeof(buf));
    if (n != 7) {
        api->puts("FAIL: nested read size\n");
        return;
    }
    buf[n] = '\0';
    api->printf("nested: %s", buf);

    /* 5. переименование */
    if (!api->fs_rename("A.TXT", "B.TXT")) {
        api->puts("FAIL: rename\n");
        return;
    }
    if (api->fs_exists("A.TXT") || !api->fs_exists("B.TXT")) {
        api->puts("FAIL: rename check\n");
        return;
    }

    /* 6. удаление */
    if (!api->fs_remove("B.TXT")) {
        api->puts("FAIL: remove\n");
        return;
    }
    if (api->fs_exists("B.TXT")) {
        api->puts("FAIL: remove check\n");
        return;
    }
    if (!api->fs_remove("DIR1/INNER.TXT")) {
        api->puts("FAIL: remove nested\n");
        return;
    }
    if (api->fs_exists("DIR1/INNER.TXT")) {
        api->puts("FAIL: nested remove check\n");
        return;
    }
    if (!api->fs_rmdir("DIR1")) {
        api->puts("FAIL: rmdir (not empty)\n");
        return;
    }

    uint32_t after = api->fs_free_space();
    api->printf("FS IO test done. free: %u -> %u bytes\n", before, after);
    api->exit(0);
}
