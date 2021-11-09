#include <device.h>
#include <eval.h>
#include <kernel.h>
#include <string.h>
#include <thread.h>
#include <timer.h>

extern void eval_void_start();

static char cmdline_buffer[1024];

EVAL_VIOD(help, "display this information")(int argc, char**argv)
{
    int cmdline_len;
    struct eval_void *eval_void_node = (struct eval_void *)&eval_void_start;

    while (eval_void_node != NULL && eval_void_node->magic == EVAL_VIOD_MAGIC)
    {
        cmdline_len = printk("%s", eval_void_node->name);
        while (cmdline_len < 10)
        {
            printk(" ");
            ++cmdline_len;
        }
        printk("- %s\n", eval_void_node->info);
        ++eval_void_node;
    }
}

static void exec_eval_viod(char *cmdline)
{
    struct eval_void *eval_void_node = (struct eval_void *)&eval_void_start;

    while (eval_void_node != NULL && eval_void_node->magic == EVAL_VIOD_MAGIC)
    {
        if (!strcmp(eval_void_node->name, cmdline))
        {
            eval_void_node->handler();
            return;
        }
        ++eval_void_node;
    }

    printk("`%s' is not defined\n", cmdline);
}

void eval()
{
    char ch;
    int i;

    printk("\n");

    for (;;)
    {
        printk("eval > ");
        i = 0;

        for (;;)
        {
            while ((ch = get_key()) == 0);

            if (ch == '\n')
            {
                break;
            }

            if (ch >= ' ' && ch <= '~')
            {
                printk("%c", ch);
                cmdline_buffer[i++] = ch;
                cmdline_buffer[i] = '\0';
            }
        }

        printk("\n");

        if (i > 0)
        {
            exec_eval_viod((char *)cmdline_buffer);
        }
    }
}
