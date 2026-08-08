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
#define RTC_B_BCD     0x04    /* 0 = двоичное представление, 1 = BCD */
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

void rtc_get_time(struct rtc_time *t)
{
    /* ждём завершения обновления времени */
    while (rtc_read(RTC_STATUS_A) & RTC_A_UIP) {
    }

    uint8_t status_b = rtc_read(RTC_STATUS_B);
    uint8_t bcd = (status_b & RTC_B_BCD) ? 1 : 0;

    uint8_t sec = rtc_read(RTC_SECONDS);
    uint8_t min = rtc_read(RTC_MINUTES);
    uint8_t hour = rtc_read(RTC_HOURS);
    uint8_t day = rtc_read(RTC_DAY);
    uint8_t month = rtc_read(RTC_MONTH);
    uint8_t year = rtc_read(RTC_YEAR);

    if (bcd) {
        sec = bcd_to_dec(sec);
        min = bcd_to_dec(min);
        day = bcd_to_dec(day);
        month = bcd_to_dec(month);
        year = bcd_to_dec(year);

        /* час: у 12-часового формата в бите 7 лежит PM-флаг */
        uint8_t hour_raw = hour;
        if (!(status_b & RTC_B_24H)) {
            uint8_t pm = hour_raw & 0x80;
            hour_raw &= 0x7F;
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
