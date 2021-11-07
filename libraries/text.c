#include <text.h>

char *to_dec_string(int decimal_number)
{
    static char result[34];

    char result_tmp[32];
    int i = 0, j = 0;

    if (decimal_number < 0)
    {
        result[i++] = '-';
        decimal_number = -decimal_number;
    }

    while (decimal_number > 0)
    {
        *(result_tmp + j) = '0' + (decimal_number % 10);
        ++j;
        decimal_number /= 10;
    }

    for (--j; j >= 0; result[i++] = result_tmp[j--]) {}

    if (i == 0)
    {
        result[i++] = '0';
    }

    result[i] = '\0';

    return result;
}

char *to_udec_string(uint32_t decimal_number)
{
    static char result[34];

    char result_tmp[32];
    int i = 0, j = 0;

    while (decimal_number > 0)
    {
        *(result_tmp + j) = '0' + (decimal_number % 10);
        ++j;
        decimal_number /= 10;
    }

    for (--j; j >= 0; result[i++] = result_tmp[j--]) {}

    if (i == 0)
    {
        result[i++] = '0';
    }

    result[i] = '\0';

    return result;
}

char *to_oct_string(uint32_t decimal_number)
{
    static char result[34];

    char result_tmp[32];
    int i = 0, j = 0;

    result[i++] = '0';

    while (decimal_number > 0)
    {
        *(result_tmp + j) = '0' + (decimal_number % 8);
        ++j;
        decimal_number >>= 3;
    }

    for (--j; j >= 0; result[i++] = result_tmp[j--]) {}

    return result;
}

char *to_hex_string(uint32_t decimal_number, bool_t sign)
{
    static char result[35];
    const char hex_char[16] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    char result_tmp[32];
    int i = 0, j = 0;

    if (sign == true)
    {
        result[i++] = '0';
        result[i++] = 'x';
    }

    if (decimal_number == 0)
    {
        result[i++] = '0';
    }
    else
    {
        while (decimal_number > 0)
        {
            *(result_tmp + j) = hex_char[decimal_number % 16];
            ++j;
            decimal_number >>= 4;
        }
    }

    for (--j; j >= 0; result[i++] = result_tmp[j--]) {}

    result[i] = '\0';

    return result;
}

char *to_bin_string(uint32_t decimal_number)
{
    static char result[34];

    char result_tmp[32];
    int i = 0, j = 0;

    while (decimal_number > 0)
    {
        *(result_tmp + j) = (decimal_number % 2) ? '1' : '0';
        ++j;
        decimal_number >>= 1;
    }

    for (--j; j >= 0; result[i++] = result_tmp[j--]) {}

    result[i++] = 'b';
    result[i]   = '\0';

    return result;
}
