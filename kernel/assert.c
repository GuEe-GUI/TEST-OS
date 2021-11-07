#include <console.h>
#include <io.h>
#include <kernel.h>
#include <vbe.h>

void __attribute__((noreturn)) assert(const char *file, uint32_t line, const char *func, const char *expression)
{
    const char *assert_hr = "------------------------------------------";
    const char *assert_title = "TEST OS ASSERT";

    cli();

    set_color(0xef2929ff, CONSOLE_CLEAR);
    printk("%s %s %s", assert_hr, assert_title, assert_hr);
    printk("file:\t\t%s\n", file);
    printk("line:\t\t%d\n", line);
    printk("function:\t%s\n", func);
    printk("expression:\t%s\n", expression);
    printk("%s %s %s", assert_hr, assert_title, assert_hr);
    set_color(CONSOLE_FILL, CONSOLE_CLEAR);

    for (;;)
    {
        hlt();
    }
}
