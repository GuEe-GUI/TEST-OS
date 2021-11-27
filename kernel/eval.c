#include <console.h>
#include <device.h>
#include <eval.h>
#include <kernel.h>
#include <string.h>
#include <thread.h>
#include <timer.h>

extern void eval_void_start();

static char cmdline_buffer[EVAL_CHAR_MAX];
static char *cmdline_args[EVAL_ARGS_MAX];

EVAL_VOID(help, "display this information")(int argc, char**argv)
{
    int cmdline_len;
    struct eval_void *node = (struct eval_void *)&eval_void_start;

    while (node != NULL && node->magic == EVAL_VOID_MAGIC)
    {
        cmdline_len = printk("%s", node->name);
        while (cmdline_len < KERNEL_THREAD_NAME_LEN)
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
    const char back_force[] = {CONSOLE_BACK_FORCE, '\0'};
    uint8_t ch;
    int i;

    printk("Welcome to eval!\n\n");

finsh:
    for (;;)
    {
        i = 0;

        console_out("eval>", EVAL_COLOR_INFO, CONSOLE_CLEAR);
        console_out(" ", CONSOLE_FILL, CONSOLE_CLEAR);

        for (;;)
        {
            while ((ch = get_key()) == 0) {}

            if (ch >= ' ' && ch <= '~' && (i >= ARRAY_SIZE(cmdline_buffer) ? (ch = '\n', i = -1, 0) : 1))
            {
                printk("%c", ch);
                cmdline_buffer[i++] = ch;
                cmdline_buffer[i] = '\0';
            }
            else if (ch == '\n')
            {
                if (i == -1)
                {
                    set_color(EVAL_COLOR_ERROR, CONSOLE_CLEAR);
                    printk("\ntoo many characters (max = %d)", ARRAY_SIZE(cmdline_buffer));
                    set_color(CONSOLE_FILL, CONSOLE_CLEAR);
                }
                printk("\n");
                break;
            }
            else if (ch == 8 && i > 0)
            {
                cmdline_buffer[--i] = '\0';
                printk((char *)back_force);
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
                else if (cmdline_buffer[idx] == '\"' && (idx > 0 && cmdline_buffer[idx - 1] != '\\'))
                {
                    if ((mark = !mark))
                    {
                        mark = idx;
                    }
                    else
                    {
                        space = 1;
                        cmdline_buffer[idx] = '\0';
                    }
                }
                else
                {
                    if (space || argc == 0)
                    {
                        cmdline_args[argc++] = &cmdline_buffer[idx];
                        if (argc >= ARRAY_SIZE(cmdline_args))
                        {
                            set_color(EVAL_COLOR_ERROR, CONSOLE_CLEAR);
                            printk("too many arguments (max = %d)\n", ARRAY_SIZE(cmdline_args));
                            set_color(CONSOLE_FILL, CONSOLE_CLEAR);
                            goto finsh;
                        }
                    }
                    space = 0;
                }
            }

            if (mark)
            {
                set_color(EVAL_COLOR_ERROR, CONSOLE_CLEAR);
                printk("missing terminating \" character\n");
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

            set_color(EVAL_COLOR_ERROR, CONSOLE_CLEAR);
            printk("`%s' is not defined\n", cmdline_buffer);
            set_color(CONSOLE_FILL, CONSOLE_CLEAR);
        }
    }
}
