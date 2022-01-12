#include <io.h>
#include <kernel.h>
#include <rtc.h>

static struct rtc_time time;

void init_rtc(void)
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

    LOG("rtc time = %d/%d/%d %d:%d:%d", time.year, time.month, time.day, time.hour, time.minute, time.second);
}

int get_update_in_progress_flag(void)
{
    io_out8(0x70, 0x0A);
    return (io_in8(0x71) & 0x80);
}

uint8_t get_rtc_register(int reg)
{
    io_out8(0x70, reg);
    return io_in8(0x71);
}

struct rtc_time *read_rtc(void)
{
    uint8_t century;
    uint8_t last_second;
    uint8_t last_minute;
    uint8_t last_hour;
    uint8_t last_day;
    uint8_t last_month;
    uint8_t last_year;
    uint8_t last_century;
    uint8_t registerB;

    while (get_update_in_progress_flag()) {}

    time.second = get_rtc_register(CMOS_CUR_SEC);
    time.minute = get_rtc_register(CMOS_CUR_MIN);
    time.hour = get_rtc_register(CMOS_CUR_HOUR);
    time.day = get_rtc_register(CMOS_MON_DAY);
    time.month = get_rtc_register(CMOS_CUR_MON);
    time.year = get_rtc_register(CMOS_CUR_YEAR);
    century = get_rtc_register(CMOS_CUR_CEN);

    do {
        last_second = time.second;
        last_minute = time.minute;
        last_hour = time.hour;
        last_day = time.day;
        last_month = time.month;
        last_year = time.year;
        last_century = century;

        while (get_update_in_progress_flag()) {}

        time.second = get_rtc_register(CMOS_CUR_SEC);
        time.minute = get_rtc_register(CMOS_CUR_MIN);
        time.hour = get_rtc_register(CMOS_CUR_HOUR);
        time.day = get_rtc_register(CMOS_MON_DAY);
        time.month = get_rtc_register(CMOS_CUR_MON);
        time.year = get_rtc_register(CMOS_CUR_YEAR);
        century = get_rtc_register(CMOS_CUR_CEN);
    } while ((last_second != time.second) ||
            (last_minute != time.minute) ||
            (last_hour != time.hour) ||
            (last_day != time.day) ||
            (last_month != time.month) ||
            (last_year != time.year) ||
            (last_century != century));

    registerB = get_rtc_register(CMOS_REG_B_SR);

    if (!(registerB & 0x04))
    {
        time.second = (time.second & 0x0F) + ((time.second / 16) * 10);
        time.minute = (time.minute & 0x0F) + ((time.minute / 16) * 10);
        time.hour = ( (time.hour & 0x0F) + (((time.hour & 0x70) / 16) * 10) ) | (time.hour & 0x80);
        time.day = (time.day & 0x0F) + ((time.day / 16) * 10);
        time.month = (time.month & 0x0F) + ((time.month / 16) * 10);
        time.year = (time.year & 0x0F) + ((time.year / 16) * 10);
        century = (century & 0x0F) + ((century / 16) * 10);
    }

    if (!(registerB & 0x02) && (time.hour & 0x80))
    {
        time.hour = ((time.hour & 0x7F) + 12) % 24;
    }

    time.year += century * 100;

    return &time;
}

unsigned long get_timestamp(void)
{
    read_rtc();

    return to_timestamp(time.year, time.month, time.day, time.hour, time.minute, time.second);
}

unsigned long to_timestamp(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t minute, uint32_t second)
{
    if (0 >= (int)(month -= 2))
    {
        month += 12;
        year -= 1;
    }

    return (((
            (unsigned long)(year / 4 - year / 100 + year / 400 + 367 * month / 12 + day) + year * 365 - 719499
            ) * 24 + hour
        ) * 60 + minute
    ) * 60 + second;
}
