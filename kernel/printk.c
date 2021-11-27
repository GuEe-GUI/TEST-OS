#include <kernel.h>
#include <stdarg.h>
#include <text.h>
#include <console.h>

int printk(const char *fmt, ...)
{
    int string_length = 0;
    /* 多线程下不允许使用static */
    char result[2048] = {0};
    va_list arg_ptr;

    va_start(arg_ptr, fmt);

    while (*fmt)
    {
        if (*fmt != '%')
        {
            result[string_length++] = *(fmt++);
            continue;
        }

        ++fmt;

        switch (*fmt)
        {
        case 'd': case 'D':
        {
            string_length += base10_string(va_arg(arg_ptr, int), 0, &result[string_length]);
            break;
        }
        case 'x': case 'p': case 'X':
        {
            string_length += base16_string(va_arg(arg_ptr, int), 0, &result[string_length]);
            break;
        }
        case 's': case 'S':
        {
            char *str = va_arg(arg_ptr, char *);
            while (*str)
            {
                result[string_length++] = *str++;
            }
            break;
        }
        case 'c': case 'C':
        {
            /* 如果是0输出会被截断，因此用空格代替 */
            char ch = (char)va_arg(arg_ptr, int);
            if (ch == 0)
            {
                ch = ' ';
            }
            result[string_length++] = ch;
            break;
        }
        case 'b': case 'B':
        {
            string_length += base2_string(va_arg(arg_ptr, int), 0, &result[string_length]);
            break;
        }
        case 'u': case 'U':
        {
            string_length += base10_string(va_arg(arg_ptr, int), 1, &result[string_length]);
            break;
        }
        case 'o': case 'O':
        {
            string_length += base8_string(va_arg(arg_ptr, int), 0, &result[string_length]);
            break;
        }
        case '%':
        {
            result[string_length++] = '%';
            break;
        }
        /* 未完待续... */
        default: result[string_length++] = *fmt; break;
        }

        ++fmt;
    }
    va_end(arg_ptr);

    result[string_length] = '\0';
    console_out(result, 0, 0);

    return string_length;
}
