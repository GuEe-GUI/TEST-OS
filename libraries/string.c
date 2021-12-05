#include <string.h>

size_t strlen(const char *str)
{
    const char *p = str;
    while (*p++);

    return (p - str - 1);
}

char strcmp(const char *str1, const char *str2)
{
    if (str1 == str2)
    {
        return 0;
    }

    while (*str1 != 0 && *str1 == *str2)
    {
        str1++;
        str2++;
    }
    return *str1 < *str2 ? -1 : *str1 > *str2;
}

void *memcpy(void *dst, const void *src, size_t size)
{
    char *d;
    const char *s;

    if (dst == NULL || src == NULL)
    {
        return NULL;
    }

    if ((char *)dst > ((char *)src + sizeof(src)) || ((char *)dst < (char *)src))
    {
        d = (char *)dst;
        s = (char *)src;
        while (size--)
        {
            *d++ = *s++;
        }
    }
    else
    {
        d = ((char *)dst + size - 1);
        s = ((char *)src + size -1);
        while (size --)
        {
            *d-- = *s--;
        }
    }

    return dst;
}

void *memset(void *dst, uint32_t val, uint32_t size)
{
    for (; 0 < size; size--)
    {
        *(char *)dst = val;
        dst++;
    }

    return dst;
}

uint32_t memcmp(const void *buf1, const void *buf2, uint32_t size)
{
    while (size --> 0)
    {
        if (*(uint32_t *)buf1++ != *(uint32_t *)buf2++)
        {
            return 0;
        }
    }
    return 1;
}

void *memmove(void *dst, const void *src, size_t n)
{
    char *tmp;
    const char *s;

    if (dst <= src)
    {
        tmp = dst;
        s = src;
        while (n--)
        {
            *tmp++ = *s++;
        }
    }
    else
    {
        tmp = dst;
        tmp += n;
        s = src;
        s += n;
        while (n--)
        {
            *--tmp = *--s;
        }
    }
    return dst;
}

void *memchr(const void *buf, int c, size_t n)
{
    char *p = (char *)buf;
    char *end = p + n;

    while (p != end)
    {
        if (*p == c)
        {
            return p;
        }

        ++p;
    }

    return 0;
}
