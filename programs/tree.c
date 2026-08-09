/*
 * tree.bin — дерево каталогов (порт из x16-PRos).
 * Usage: tree [dir]
 */
#include "api/sysapi.h"

/* Имя записи из строки fs_list "NAME.EXT SIZE\n" ("DIR/ SIZE\n").
 * Возвращает длину имени; *is_dir = признак каталога. */
static uint32_t parse_entry(const char *line, char *name, int *is_dir)
{
    uint32_t n = 0;
    *is_dir = 0;
    while (line[n] && line[n] != ' ' && n < 63) {
        name[n] = line[n];
        n++;
    }
    name[n] = '\0';
    if (n > 0 && name[n - 1] == '/') {
        name[n - 1] = '\0';
        *is_dir = 1;
        n--;
    }
    return n;
}

static void tree_rec(struct neopros_api *api, const char *path,
                     uint32_t depth, uint32_t *count)
{
    if (depth > 5) {
        return;
    }
    static char list_buf[2048];
    uint32_t entries = api->fs_list(path, list_buf, sizeof(list_buf));
    if (entries == 0xFFFFFFFFu) {
        return;
    }

    char line[512];
    uint32_t li = 0;
    for (uint32_t i = 0; ; i++) {
        if (list_buf[i] == '\n' || list_buf[i] == '\0') {
            if (li > 0) {
                line[li] = '\0';
                char name[64];
                int is_dir;
                parse_entry(line, name, &is_dir);

                for (uint32_t d = 0; d < depth; d++) {
                    api->puts("  ");
                }
                api->puts(name);
                api->puts(is_dir ? "/\n" : "\n");
                (*count)++;

                if (is_dir) {
                    char sub[128];
                    uint32_t p = 0;
                    if (path[0] != '\0') {
                        while (path[p] && p < 63) {
                            sub[p] = path[p];
                            p++;
                        }
                        sub[p++] = '/';
                    }
                    for (uint32_t j = 0; name[j] && p < 127; j++) {
                        sub[p++] = name[j];
                    }
                    sub[p] = '\0';
                    tree_rec(api, sub, depth + 1, count);
                }
                li = 0;
            }
            if (list_buf[i] == '\0') {
                break;
            }
        } else if (li < sizeof(line) - 1) {
            line[li++] = list_buf[i];
        }
    }
}

void _start(struct neopros_api *api, int argc, char **argv)
{
    const char *root = (argc > 1) ? argv[1] : "";
    uint32_t count = 0;
    api->puts(root[0] == '\0' ? "/\n" : root);
    api->puts("\n");
    tree_rec(api, root, 1, &count);
    api->printf("%u entries\n", count);
}
