#include <io.h>
#include <kernel.h>
#include <rtc.h>

#define CURRENT_YEAR 2020
#define CENTURY_REG  0x00

static struct rtc_time time;

static inline void nmi_enable(void)
{
    io_out8(0x70, io_in8(0x70) & 0x7f);
}

static inline void nmi_disable(void)
{
    io_out8(0x70, io_in8(0x70) | 0x80);
}

void init_rtc()
{
    cli();                      /* 关中断 */

    io_out8(0x70, 0x8a);        /* 选择状态寄存器A，并禁用NMI（通过将0x80位置1） */
    io_out8(0x71, 0x20);        /* 写入CMOS / RTC RAM */

    io_out8(0x70, 0x8b);        /* 选择寄存器B，并禁用NMI */
    char prev = io_in8(0x71);   /* 读取寄存器B的当前值 */
    io_out8(0x70, 0x8b);        /* 再次设置索引（读取将重置索引以进行注册 */
    io_out8(0x71, prev | 0x40); /* 写入先前的值与0x40进行“或”运算。 这将打开第6位 */

    sti();                      /* 开中断 */

    read_rtc();

    LOG("rtc time = %d/%d/%d %d:%d:%d\n", time.year, time.month, time.day, time.hour, time.minute, time.second);
}

int get_update_in_progress_flag()
{
    io_out8(0x70, 0x0A);
    return (io_in8(0x71) & 0x80);
}

unsigned char get_rtc_register(int reg)
{
    io_out8(0x70, reg);
    return io_in8(0x71);
}

struct rtc_time *read_rtc()
{
    unsigned char century = 20;
    unsigned char last_second;
    unsigned char last_minute;
    unsigned char last_hour;
    unsigned char last_day;
    unsigned char last_month;
    unsigned char last_year;
    unsigned char last_century;
    unsigned char registerB;

    while (get_update_in_progress_flag()) {}

    time.second = get_rtc_register(0x00);
    time.minute = get_rtc_register(0x02);
    time.hour = get_rtc_register(0x04);
    time.day = get_rtc_register(0x07);
    time.month = get_rtc_register(0x08);
    time.year = get_rtc_register(0x09);

    if (CENTURY_REG != 0)
    {
        century = get_rtc_register(CENTURY_REG);
    }

    do {
        last_second = time.second;
        last_minute = time.minute;
        last_hour = time.hour;
        last_day = time.day;
        last_month = time.month;
        last_year = time.year;
        last_century = century;

        while (get_update_in_progress_flag()) {}

        time.second = get_rtc_register(0x00);
        time.minute = get_rtc_register(0x02);
        time.hour = get_rtc_register(0x04);
        time.day = get_rtc_register(0x07);
        time.month = get_rtc_register(0x08);
        time.year = get_rtc_register(0x09);

        if(CENTURY_REG != 0)
        {
            century = get_rtc_register(CENTURY_REG);
        }
    } while((last_second != time.second) ||
            (last_minute != time.minute) ||
            (last_hour != time.hour) ||
            (last_day != time.day) ||
            (last_month != time.month) ||
            (last_year != time.year) ||
            (last_century != century));

    registerB = get_rtc_register(0x0B);

    if (!(registerB & 0x04))
    {
        time.second = (time.second & 0x0F) + ((time.second / 16) * 10);
        time.minute = (time.minute & 0x0F) + ((time.minute / 16) * 10);
        time.hour = ( (time.hour & 0x0F) + (((time.hour & 0x70) / 16) * 10) ) | (time.hour & 0x80);
        time.day = (time.day & 0x0F) + ((time.day / 16) * 10);
        time.month = (time.month & 0x0F) + ((time.month / 16) * 10);
        time.year = (time.year & 0x0F) + ((time.year / 16) * 10);

        if(CENTURY_REG != 0)
        {
            century = (century & 0x0F) + ((century / 16) * 10);
        }
    }

    if (!(registerB & 0x02) && (time.hour & 0x80))
    {
        time.hour = ((time.hour & 0x7F) + 12) % 24;
    }

    if(CENTURY_REG != 0)
    {
        time.year += century * 100;
    }
    else
    {
        time.year += (CURRENT_YEAR / 100) * 100;
        if(time.year < CURRENT_YEAR)
        {
            time.year += 100;
        }
    }

    return &time;
}
