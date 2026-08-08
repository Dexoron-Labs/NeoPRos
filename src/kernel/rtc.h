#ifndef NEOPROS_RTC_H
#define NEOPROS_RTC_H

#include "multiboot.h"

/* Дата и время из RTC (CMOS) */
struct rtc_time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

/* Чтение текущих даты и времени из CMOS RTC. */
void rtc_get_time(struct rtc_time *t);

#endif /* NEOPROS_RTC_H */
