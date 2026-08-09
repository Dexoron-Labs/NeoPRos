/*
 * procentc.bin — процентный калькулятор (порт из x16-PRos).
 * Интерактивный: вводится значение и процент; "0" — выход.
 */
#include "api/sysapi.h"

static int atoi_simple(const char *s)
{
    int v = 0;
    int neg = 0;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    return neg ? -v : v;
}

void _start(struct neopros_api *api, int argc, char **argv)
{
    (void)argc;
    (void)argv;

    char input[32];
    api->puts("Welcome to the Percentage Calculator!\n");
    api->puts("Calculates: P% of X = result.\n");
    api->puts("Enter 0 as value to quit.\n");

    for (;;) {
        api->readline("Value: ", input, sizeof(input));
        int x = atoi_simple(input);
        if (x == 0) {
            break;
        }
        api->readline("Percent: ", input, sizeof(input));
        int p = atoi_simple(input);
        api->printf("%d%% of %d = %d.%d\n", p, x,
                    x * p / 100, x * p % 100);
    }
    api->puts("Goodbye!\n");
}
