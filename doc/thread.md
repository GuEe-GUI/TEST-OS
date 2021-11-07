# 线程案例

## 代码
```c
#include <kernel.h>
#include <thread.h>
#include <timer.h>

void printA()
{
    for (;;)
    {
        set_color(0xff0000ff, CONSOLE_CLEAR);
        printk("A ");
        set_color(CONSOLE_FILL, CONSOLE_CLEAR);
        delay(100);
    }
}

void printB()
{
    for (;;)
    {
        set_color(0x00ff00ff, CONSOLE_CLEAR);
        printk("B ");
        set_color(CONSOLE_FILL, CONSOLE_CLEAR);
        delay(100);
    }
}

void printC()
{
    for (;;)
    {
        set_color(0x0000ffff, CONSOLE_CLEAR);
        printk("C ");
        set_color(CONSOLE_FILL, CONSOLE_CLEAR);
        delay(100);
    }
}

void printD()
{
    for (;;)
    {
        set_color(0x00ffffff, CONSOLE_CLEAR);
        printk("D ");
        set_color(CONSOLE_FILL, CONSOLE_CLEAR);
        delay(100);
    }
}

void printE()
{
    for (;;)
    {
        set_color(0xffff00ff, CONSOLE_CLEAR);
        printk("E ");
        set_color(CONSOLE_FILL, CONSOLE_CLEAR);
        delay(100);
    }
}

int main()
{
    tid_t tida, tidb, tidc, tidd, tide;

    thread_create(&tida, printA, NULL);
    thread_create(&tidb, printB, NULL);
    thread_create(&tidc, printC, NULL);
    thread_create(&tidd, printD, NULL);
    thread_create(&tide, printE, NULL);

    thread_setup(tide);
    thread_setup(tidd);
    thread_setup(tidc);
    thread_setup(tidb);
    thread_setup(tida);

    while (1);

    return 0;
}
```

## 截图
![截图](https://github.com/DoctorMrGUI/TEST-OS/raw/master/doc/thread.png "截图")