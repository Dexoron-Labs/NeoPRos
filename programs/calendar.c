/*
 * calendar.bin — календарь текущего месяца (порт из x16-PRos).
 * Неделя начинается с понедельника.
 */
#include "api/sysapi.h"

static const char *month_names[12] = {
    "January", "February", "March",     "April",   "May",      "June",
    "July",    "August",   "September", "October", "November", "December",
};

/* Дни с 2000-01-01 (суббота). */
static long days_since_2000(int y, int m, int d)
{
    static const int mdays[12] = {31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31};
    long days = 0;
    for (int yy = 2000; yy < y; yy++) {
        days += (yy % 4 == 0 && (yy % 100 != 0 || yy % 400 == 0)) ? 366 : 365;
    }
    for (int mm = 0; mm < m - 1; mm++) {
        days += mdays[mm];
    }
    if (m > 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))) {
        days++;
    }
    return days + d - 1;
}

/* День недели: 0 = воскресенье, 1 = понедельник, ... */
static int day_of_week(int y, int m, int d)
{
    return (int)((6 + days_since_2000(y, m, d)) % 7);
}

static int days_in_month(int y, int m)
{
    static const int mdays[12] = {31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31};
    if (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))) {
        return 29;
    }
    return mdays[m - 1];
}

void _start(struct neopros_api *api, int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct neopros_time t;
    api->rtc(&t);

    api->printf("     %s %u\n", month_names[t.month - 1], t.year);
    api->puts("Mo Tu We Th Fr Sa Su\n");

    int first = day_of_week(t.year, t.month, 1);   /* 0 = вс */
    int col = (first == 0) ? 6 : first - 1;        /* 0 = пн */
    for (int i = 0; i < col; i++) {
        api->puts("   ");
    }
    int dim = days_in_month(t.year, t.month);
    for (int d = 1; d <= dim; d++) {
        api->putc(' ');
        api->putc((char)('0' + d / 10));
        api->putc((char)('0' + d % 10));
        col++;
        if (col % 7 == 0) {
            api->putc('\n');
        }
    }
    if (col % 7 != 0) {
        api->putc('\n');
    }
    api->printf("Today: %02u-%02u-%04u\n", t.day, t.month, t.year);
}
