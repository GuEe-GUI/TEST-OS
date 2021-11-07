#include <device.h>
#include <kernel.h>
#include <thread.h>
#include <timer.h>

void eval()
{
    char ch;

    for (;;)
    {
        printk("\neval > ");

        for (;;)
        {
            while ((ch = get_key()) == 0);

            if (ch == '\n')
            {
                break;
            }

            printk("%c", ch);
        }
    }
}
