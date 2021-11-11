#include <device.h>
#include <eval.h>
#include <kernel.h>
#include <string.h>
#include <thread.h>
#include <timer.h>

extern void eval_void_start();

static char cmdline_buffer[1024];
static char *cmdline_args[32];

EVAL_VOID(help, "display this information")(int argc, char**argv)
{
    int cmdline_len;
    struct eval_void *node = (struct eval_void *)&eval_void_start;

    while (node != NULL && node->magic == EVAL_VOID_MAGIC)
    {
        cmdline_len = printk("%s", node->name);
        while (cmdline_len < 10)
        {
            printk(" ");
            ++cmdline_len;
        }
        printk("- %s\n", node->info);
        ++node;
    }
}

void eval()
{
    char ch;
    int i;

    printk("Welcome to eval!\n\n");

finsh:
    for (;;)
    {
        i = 0;

        set_color(0x1583d7ff, CONSOLE_CLEAR);
        printk("eval >");
        set_color(CONSOLE_FILL, CONSOLE_CLEAR);
        printk(" ");

        for (;;)
        {
            while ((ch = get_key()) == 0);

            if (ch >= ' ' && ch <= '~')
            {
                printk("%c", ch);
                cmdline_buffer[i++] = ch;
                cmdline_buffer[i] = '\0';
            }
            else if (ch == '\n')
            {
                printk("\n");
                break;
            }
            else if (ch == 8 && i > 0)
            {
                cmdline_buffer[--i] = '\0';
                printk("\b \b");
            }
        }

        if (i > 0)
        {
            int idx = 0;
            int argc = 0;
            int mark = 0, space = 0;
            struct eval_void *node = (struct eval_void *)&eval_void_start;

            for (; cmdline_buffer[idx] != '\0'; ++idx)
            {
                if (cmdline_buffer[idx] == ' ')
                {
                    if (!mark)
                    {
                        space = 1;
                        cmdline_buffer[idx] = '\0';
                    }
                }
                else if (cmdline_buffer[idx] == '\"')
                {
                    mark = !mark;
                    if (!space)
                    {
                        cmdline_buffer[idx] = '\0';
                    }
                }
                else
                {
                    if (space || argc == 0)
                    {
                        cmdline_args[argc++] = &cmdline_buffer[idx];
                        if (argc >= sizeof(cmdline_args) / sizeof(cmdline_args[0]))
                        {
                            set_color(0xf44336ff, CONSOLE_CLEAR);
                            printk("argc out of range(%d) error\n", sizeof(cmdline_args) / sizeof(cmdline_args[0]));
                            set_color(CONSOLE_FILL, CONSOLE_CLEAR);
                            goto finsh;
                        }
                    }
                    space = 0;
                }
            }

            if (mark)
            {
                set_color(0xf44336ff, CONSOLE_CLEAR);
                printk("mark matching error\n");
                set_color(CONSOLE_FILL, CONSOLE_CLEAR);
                continue;
            }
            cmdline_args[argc] = NULL;

            while (node != NULL && node->magic == EVAL_VOID_MAGIC)
            {
                if (!strcmp(node->name, cmdline_args[0]))
                {
                    node->handler(argc, cmdline_args);
                    goto finsh;
                }
                ++node;
            }

            printk("`%s' is not defined\n", cmdline_buffer);
        }
    }
}
