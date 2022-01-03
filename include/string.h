#ifndef _STRING_H
#define _STRING_H

#include <types.h>

size_t strlen(const char *str);
char strcmp(const char *str1, const char *str2);
char *strcpy(char *str1, const char *str2);
char *strncpy(char *str1, const char *str2, size_t n);
char *strchr(const char *str, int c);
char *strrchr(const char *str, int c);
char *strtok(char *str, char *delim);

void *memcpy(void *dst, const void *src, size_t size);
void *memset(void *dst, uint32_t val, uint32_t size);
uint32_t memcmp(const void *buf1, const void *buf2, uint32_t size);
void *memmove(void *dst, const void *src, size_t size);
void *memchr(const void *buf, int c, size_t size);

#endif /* _STRING_H */
