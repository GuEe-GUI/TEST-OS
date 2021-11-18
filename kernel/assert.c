#include <console.h>
#include <io.h>
#include <kernel.h>
#include <string.h>
#include <vbe.h>

static void print_hr(int len)
{
    while (len --> 0)
    {
        printk("-");
    }
}

void __attribute__((noreturn)) assert(const char *file, uint32_t line, const char *func, const char *expression)
{
    const char *assert_title = " TEST OS ASSERT ";
    int assert_hr_len = (get_console_cols() - strlen(assert_title)) / 2;

    cli();

    set_color(0xef2929ff, CONSOLE_CLEAR);
    print_hr(assert_hr_len);
    printk(assert_title);
    print_hr(assert_hr_len);

    printk("file:\t\t%s\n", file);
    printk("line:\t\t%d\n", line);
    printk("function:\t%s\n", func);
    printk("expression:\t%s\n", expression);

    print_hr(assert_hr_len);
    printk(assert_title);
    print_hr(assert_hr_len);

    for (;;)
    {
        hlt();
    }
}
