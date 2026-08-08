#ifndef NEOPROS_STRING_H
#define NEOPROS_STRING_H

#include "multiboot.h"

void *memset(void *dst, int value, uint32_t size);
void *memcpy(void *dst, const void *src, uint32_t size);
uint32_t strlen(const char *str);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, uint32_t n);
void strcpy(char *dst, const char *src);
char to_upper(char c);
char to_lower(char c);
uint32_t atoi(const char *str);

#endif /* NEOPROS_STRING_H */
