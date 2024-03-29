# 线程案例

## 代码
```c
#include <kernel.h>
#include <thread.h>
#include <timer.h>
#include <eval.h>

void print_char(void *param)
{
    char ch = (uint32_t)PTR_LIST_ITEM(param, 0);
    uint32_t color = (uint32_t)PTR_LIST_ITEM(param, 1);

    for (;;)
    {
        set_color(color, CONSOLE_CLEAR);
        printk("%c ", ch);
        set_color(CONSOLE_FILL, CONSOLE_CLEAR);
        delay(100);
    }
}

EVAL_VOID(thread_demo, "thread demo")(int argc, char**argv)
{
    tid_t tid[5];

    thread_create(&tid[0], NULL, print_char, PTR_LIST((char *)'A', (char *)0xff0000ff));
    thread_create(&tid[1], NULL, print_char, PTR_LIST((char *)'B', (char *)0x00ff00ff));
    thread_create(&tid[2], NULL, print_char, PTR_LIST((char *)'C', (char *)0x0000ffff));
    thread_create(&tid[3], NULL, print_char, PTR_LIST((char *)'D', (char *)0x00ffffff));
    thread_create(&tid[4], NULL, print_char, PTR_LIST((char *)'E', (char *)0xffff00ff));

    thread_wake(tid[4]);
    thread_wake(tid[3]);
    thread_wake(tid[2]);
    thread_wake(tid[1]);
    thread_wake(tid[0]);

    while (1);
}
```

## 截图
![截图](thread.png "截图")