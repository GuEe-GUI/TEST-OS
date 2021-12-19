#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <console.h>
#include <io.h>
#include <vbe.h>

#ifdef __DEBUG__
#include <timer.h>

#define LOG(fmt, ...) \
do { \
    set_color(0xffc107ff, CONSOLE_CLEAR); \
    print_tick(); \
    printk(" " fmt, ##__VA_ARGS__); \
    set_color(CONSOLE_FILL, CONSOLE_CLEAR); \
} while (0)

#else
#define LOG(...)
#endif /* __DEBUG__ */

#define ASSERT(expression) \
if (!(expression)) \
{ \
    assert(__FILE__, __LINE__, __func__, #expression); \
}

#define PANIC(fmt, ...) \
do { \
    cli(); \
    set_color(0xbc81b5ff, CONSOLE_CLEAR); \
    printk(fmt, ##__VA_ARGS__); \
    ASSERT(0); \
} while (0)

int printk(const char *fmt, ...);
void __attribute__((noreturn)) assert(const char *file, uint32_t line, const char *func, const char *expression);

#endif /* _KERNEL_H_ */
