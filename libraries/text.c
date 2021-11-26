#include <text.h>

int base10_string(uint32_t number, unsigned char sign, char dst[])
{
    char tmp[32];
    int i = 0, j = 0;

    if (sign && (int)number < 0)
    {
        dst[i++] = '-';
        number &= 0x7fffffff;
    }

    while (number > 0)
    {
        *(tmp + j) = '0' + (number % 10);
        ++j;
        number /= 10;
    }

    for (--j; j >= 0; dst[i++] = tmp[j--]) {}

    if (i == 0)
    {
        dst[i++] = '0';
    }

    dst[i] = '\0';

    return i;
}

int base8_string(uint32_t number, unsigned char sign, char dst[])
{
    char tmp[32];
    int i = 0, j = 0;

    if (sign && (int)number < 0)
    {
        dst[i++] = '-';
        number &= 0x7fffffff;
    }

    while (number > 0)
    {
        *(tmp + j) = '0' + (number % 8);
        ++j;
        number >>= 3;
    }

    for (--j; j >= 0; dst[i++] = tmp[j--]) {}

    return i;
}

int base16_string(uint32_t number, unsigned char sign, char dst[])
{
    const char hex_char[16] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    char tmp[32];
    int i = 0, j = 0;

    if (sign && (int)number < 0)
    {
        dst[i++] = '-';
        number &= 0x7fffffff;
    }

    if (number == 0)
    {
        dst[i++] = '0';
    }
    else
    {
        while (number > 0)
        {
            *(tmp + j) = hex_char[number % 16];
            ++j;
            number >>= 4;
        }
    }

    for (--j; j >= 0; dst[i++] = tmp[j--]) {}

    dst[i] = '\0';

    return i;
}

int base2_string(uint32_t number, unsigned char sign, char dst[])
{
    char tmp[32];
    int i = 0, j = 0;

    if (sign && (int)number < 0)
    {
        dst[i++] = '-';
        number &= 0x7fffffff;
    }

    while (number > 0)
    {
        *(tmp + j) = (number % 2) ? '1' : '0';
        ++j;
        number >>= 1;
    }

    for (--j; j >= 0; dst[i++] = tmp[j--]) {}

    dst[i]   = '\0';

    return i;
}
