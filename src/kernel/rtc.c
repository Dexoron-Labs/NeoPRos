#include "rtc.h"
#include "io.h"

/* Регистры RTC в CMOS (0x70 — индекс, 0x71 — данные) */
#define RTC_SECONDS  0x00
#define RTC_MINUTES  0x02
#define RTC_HOURS    0x04
#define RTC_DAY      0x07
#define RTC_MONTH    0x08
#define RTC_YEAR     0x09
#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

/* Биты регистра статуса B */
#define RTC_B_DM      0x04    /* DM (data mode): 1 = двоичное, 0 = BCD */
#define RTC_B_24H     0x02    /* 1 = 24-часовой формат */

/* Бит 7 статуса A: идёт обновление времени */
#define RTC_A_UIP     0x80

/* Чтение регистра (старший бит 0x70 — запрет NMI) */
static uint8_t rtc_read(uint8_t reg)
{
    outb(0x70, (uint8_t)(0x80 | reg));
    return inb(0x71);
}

/* Перевод из BCD в десятичное значение */
static uint8_t bcd_to_dec(uint8_t value)
{
    return (uint8_t)((value & 0x0F) + (value >> 4) * 10);
}

/*
 * Принудительно включает BCD-режим и 24-часовой формат.
 * ВАЖНО: бит DM в статусе B работает наоборот, чем кажется:
 * 1 = двоичный режим, 0 = BCD. Поэтому для BCD его надо СБРАСЫВАТЬ,
 * а не устанавливать.
 */
void rtc_init(void)
{
    uint8_t status_b = rtc_read(RTC_STATUS_B);

    status_b &= (uint8_t)~RTC_B_DM;
    status_b |= RTC_B_24H;
    outb(0x70, (uint8_t)(0x80 | RTC_STATUS_B));
    outb(0x71, status_b);
}

void rtc_get_time(struct rtc_time *t)
{
    uint8_t status_b = rtc_read(RTC_STATUS_B);
    uint8_t bcd = (status_b & RTC_B_DM) ? 0 : 1;
    uint8_t sec, min, hour, day, month, year;
    uint8_t s2 = 0, m2 = 0, h2 = 0, d2 = 0, mo2 = 0, y2 = 0;
    int attempt;

    /*
     * Ожидание UIP недостаточно: обновление RTC может начаться
     * сразу после проверки (QEMU 11 в этот момент отдаёт мусорные
     * значения — например, "секунды" 0x48 вместо 0x41).
     * Поэтому читаем регистры дважды и повторяем, пока оба чтения
     * не совпадут (смена секунды между чтениями почти исключена).
     */
    for (attempt = 0; attempt < 4; attempt++) {
        while (rtc_read(RTC_STATUS_A) & RTC_A_UIP) {
        }

        uint8_t s1 = rtc_read(RTC_SECONDS);
        uint8_t m1 = rtc_read(RTC_MINUTES);
        uint8_t h1 = rtc_read(RTC_HOURS);
        uint8_t d1 = rtc_read(RTC_DAY);
        uint8_t mo1 = rtc_read(RTC_MONTH);
        uint8_t y1 = rtc_read(RTC_YEAR);

        s2 = rtc_read(RTC_SECONDS);
        m2 = rtc_read(RTC_MINUTES);
        h2 = rtc_read(RTC_HOURS);
        d2 = rtc_read(RTC_DAY);
        mo2 = rtc_read(RTC_MONTH);
        y2 = rtc_read(RTC_YEAR);

        if (s1 == s2 && m1 == m2 && h1 == h2 && d1 == d2 &&
            mo1 == mo2 && y1 == y2) {
            sec = s1;
            min = m1;
            hour = h1;
            day = d1;
            month = mo1;
            year = y1;
            break;
        }
    }
    if (attempt == 4) {
        /* все попытки неудачны: берём последние прочитанные значения */
        sec = s2;
        min = m2;
        hour = h2;
        day = d2;
        month = mo2;
        year = y2;
    }

    if (bcd) {
        sec = bcd_to_dec(sec);
        min = bcd_to_dec(min);
        day = bcd_to_dec(day);
        month = bcd_to_dec(month);
        year = bcd_to_dec(year);

        /* час: у 12-часового формата в бите 7 лежит PM-флаг */
        uint8_t hour_raw = hour & 0x7F;
        if (!(status_b & RTC_B_24H)) {
            uint8_t pm = hour & 0x80;
            hour_raw = bcd_to_dec(hour_raw);
            if (pm && hour_raw < 12) {
                hour_raw += 12;
            } else if (!pm && hour_raw == 12) {
                hour_raw = 0;
            }
        } else {
            hour_raw = bcd_to_dec(hour_raw);
        }
        hour = hour_raw;
    } else {
        /* 12-часовой формат с PM-битом */
        if (!(status_b & RTC_B_24H)) {
            uint8_t pm = hour & 0x80;
            hour &= 0x7F;
            if (pm && hour < 12) {
                hour += 12;
            } else if (!pm && hour == 12) {
                hour = 0;
            }
        }
    }

    t->second = sec;
    t->minute = min;
    t->hour = hour;
    t->day = day;
    t->month = month;
    t->year = (uint16_t)(2000 + year);
}
