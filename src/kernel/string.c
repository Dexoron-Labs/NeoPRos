#include "string.h"

void *memset(void *dst, int value, uint32_t size)
{
    uint8_t *d = (uint8_t *)dst;
    while (size--) {
        *d++ = (uint8_t)value;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, uint32_t size)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (size--) {
        *d++ = *s++;
    }
    return dst;
}

uint32_t strlen(const char *str)
{
    uint32_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

int strncmp(const char *a, const char *b, uint32_t n)
{
    while (n > 0 && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

void strcpy(char *dst, const char *src)
{
    while ((*dst++ = *src++)) {
    }
}

char to_upper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

char to_lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

uint32_t atoi(const char *str)
{
    uint32_t value = 0;
    while (*str >= '0' && *str <= '9') {
        value = value * 10 + (uint32_t)(*str - '0');
        str++;
    }
    return value;
}
