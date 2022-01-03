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

char *strcpy(char *str1, const char *str2)
{
    char *save = str1;

    for (; (*str1 = *str2); ++str2, ++str1);

    return save;
}

char *strncpy(char *str1, const char *str2, size_t n)
{
    if (n != 0)
    {
        char *d = str1;
        const char *s = str2;

        do {
            if ((*d++ = *s++) == 0)
            {
                while (--n != 0)
                {
                    *d++ = 0;
                }
                break;
            }
        } while (--n != 0);
    }

    return str1;
}

char *strchr(const char *str, int c)
{
    const unsigned char *char_ptr;
    const unsigned long int *longword_ptr;
    unsigned long int longword, magic_bits, charmask;
    unsigned char ch = (unsigned char)c;

    for (char_ptr = (const unsigned char *)str;
        ((unsigned long int)char_ptr & (sizeof(longword) - 1)) != 0;
        ++char_ptr)
    {
        if (*char_ptr == ch)
        {
            return (void *) char_ptr;
        }
        else if (*char_ptr == '\0')
        {
            return NULL;
        }
    }

    longword_ptr = (unsigned long int *)char_ptr;
    magic_bits = -1;
    magic_bits = magic_bits / 0xff * 0xfe << 1 >> 1 | 1;

    charmask = ch | (ch << 8);
    charmask |= charmask << 16;

    if (sizeof(longword) > 4)
    {
        charmask |= (charmask << 16) << 16;
    }

    typedef int longword_size_larger_than_8[sizeof(longword) > 8 ? -1 : 1] __attribute__ ((unused));

    for (;;)
    {
        longword = *longword_ptr++;
        if ((((longword + magic_bits) ^ ~longword) & ~magic_bits) != 0 ||
            ((((longword ^ charmask) + magic_bits) ^ ~(longword ^ charmask)) & ~magic_bits) != 0)
        {
            const unsigned char *cp = (const unsigned char *)(longword_ptr - 1);

            if (*cp == ch)
            {
                return (char *)cp;
            }
            else if (*cp == '\0')
            {
                return NULL;
            }
            if (*++cp == ch)
            {
                return (char *)cp;
            }
            else if (*cp == '\0')
            {
                return NULL;
            }
            if (*++cp == ch)
            {
                return (char *)cp;
            }
            else if (*cp == '\0')
            {
                return NULL;
            }
            if (*++cp == ch)
            {
                return (char *)cp;
            }
            else if (*cp == '\0')
            {
                return NULL;
            }
            if (sizeof(longword) > 4)
            {
                if (*++cp == ch)
                {
                    return (char *)cp;
                }
                else if (*cp == '\0')
                {
                    return NULL;
                }
                if (*++cp == ch)
                {
                    return (char *)cp;
                }
                else if (*cp == '\0')
                {
                    return NULL;
                }
                if (*++cp == ch)
                {
                    return (char *)cp;
                }
                else if (*cp == '\0')
                {
                    return NULL;
                }
                if (*++cp == ch)
                {
                    return (char *)cp;
                }
                else if (*cp == '\0')
                {
                    return NULL;
                }
            }
        }
    }
    return NULL;
}

char *strrchr(const char *str, int c)
{
    char *ret = 0;

    do {
        if (*str == c)
        {
            ret = (char*) str;
        }
    } while (*str++);

    return ret;
}

char *strtok(char *str, char *delim)
{
    char *spanp;
    int c, sc;
    char *tok;
    static char *last;

    if (str == NULL && (str = last) == NULL)
    {
        return NULL;
    }

cont:
    c = *str++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;)
    {
        if (c == sc)
        {
            goto cont;
        }
    }

    if (c == 0)
    {
        last = NULL;
        return NULL;
    }

    tok = str - 1;

    for (;;)
    {
        c = *str++;
        spanp = (char *)delim;

        do {
            if ((sc = *spanp++) == c)
            {
                if (c == 0)
                {
                    str = NULL;
                }
                else
                {
                    str[-1] = 0;
                }
                last = str;
                return tok;
            }
        } while (sc != 0);
    }
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
        if (*(uint8_t *)buf1++ != *(uint8_t *)buf2++)
        {
            return 1;
        }
    }
    return 0;
}

void *memmove(void *dst, const void *src, size_t size)
{
    char *tmp;
    const char *s;

    if (dst <= src)
    {
        tmp = dst;
        s = src;
        while (size--)
        {
            *tmp++ = *s++;
        }
    }
    else
    {
        tmp = dst;
        tmp += size;
        s = src;
        s += size;
        while (size--)
        {
            *--tmp = *--s;
        }
    }
    return dst;
}

void *memchr(const void *buf, int c, size_t size)
{
    char *p = (char *)buf;
    char *end = p + size;

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
