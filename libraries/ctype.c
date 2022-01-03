#include <ctype.h>

int toupper(int c)
{
    if (c >= 'a' && c <= 'z')
    {
        return c & '_';
    }

    return -1;
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c | ' ';
    }

    return -1;
}

int isspace(int c)
{
    return (c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' ' ? 1 : 0);
}

int isprint(int c)
{
    return (c >= ' ' && c <= '~');
}
