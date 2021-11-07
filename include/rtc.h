#ifndef _RTC_H_
#define _RTC_H_

struct rtc_time
{
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned int year;
};

void init_rtc();
int get_update_in_progress_flag(void);
unsigned char get_rtc_register(int reg);
struct rtc_time *read_rtc(void);

#endif
