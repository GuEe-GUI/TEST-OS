#include <console.h>
#include <device.h>
#include <disk.h>
#include <eval.h>
#include <kernel.h>
#include <string.h>
#include <thread.h>
#include <timer.h>
#include <version.h>

extern void eval_void_start();

static char cmdline_buffer[EVAL_CHAR_MAX];
static char *cmdline_args[EVAL_ARGS_MAX];
static char current_path[KERNEL_DISK_MAX_PATH];

EVAL_VOID(help, "Display this information")(int argc, char**argv)
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

char *get_eval_path()
{
    static char current_path_cache[sizeof(current_path)];

    strcpy(current_path_cache, current_path);

    return &current_path_cache[0];
}

int set_eval_path(char *path)
{
    if (path != NULL)
    {
        struct dir dir;

        if (!disk_dir_open(path, &dir))
        {
            size_t length = strlen(path);

            strcpy(current_path, path);
            disk_dir_close(&dir);

            if (length > 0 && current_path[length - 1] != '/')
            {
                current_path[length++] = '/';
                current_path[length] = '\0';
            }

            return 0;
        }
    }

    return -1;
}

void eval()
{
    const char back_force[] = {CONSOLE_BACK_FORCE, '\0'};
    uint8_t ch;
    int i;

    memset(current_path, 0, sizeof(current_path));

    for (i = KERNEL_DISK_ID_START; i < KERNEL_DISK_ID_END; ++i)
    {
        struct dir dir;
        *((uint16_t *)&current_path[0]) = i;
        current_path[sizeof(uint16_t)] = '/';

        if (!disk_dir_open(current_path, &dir))
        {
            disk_dir_close(&dir);
            break;
        }
    }

    if (i >= KERNEL_DISK_ID_END)
    {
        strcpy(current_path, "(NULL)");
    }

    printk("Welcome to eval v"OS_VERSION".\nType \"help\" for more information.\n\n");

finsh:
    for (;;)
    {
        i = 0;

        console_out(OS_ADMIN_NAME"@eval ", EVAL_COLOR_INFO, CONSOLE_CLEAR);
        console_out(get_eval_path(), EVAL_COLOR_WARN, CONSOLE_CLEAR);
        console_out(">", EVAL_COLOR_WARN, CONSOLE_CLEAR);
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
            int cmd_start_idx;
            int idx = 0;
            int argc = 0;
            int mark = 0, space = 0;
            struct eval_void *node = (struct eval_void *)&eval_void_start;

            for (; cmdline_buffer[idx] == ' '; ++idx) {}

            cmd_start_idx = idx;

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
            printk("`%s' is not defined\n", &cmdline_buffer[cmd_start_idx]);
            set_color(CONSOLE_FILL, CONSOLE_CLEAR);
        }
    }
}
