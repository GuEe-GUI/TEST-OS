#ifndef _RTC_H_
#define _RTC_H_

#include <types.h>

#define CMOS_CUR_SEC    0x0     /* CMOS当前秒：0~59 */
#define CMOS_ALA_SEC    0x1     /* CMOS闹钟秒：0~59 */
#define CMOS_CUR_MIN    0x2     /* CMOS当前分：0~59 */
#define CMOS_CUR_MIN    0x2     /* CMOS闹钟分：0~59 */
#define CMOS_CUR_HOUR   0x4     /* CMOS当前时：0~23（24小时制）或 1–12（12小时制） */
#define CMOS_ALA_HOUR   0x5     /* CMOS闹钟时：0~23（24小时制）或 1–12（12小时制） */
#define CMOS_WEEK_DAY   0x6     /* CMOS一周中当前日：1–7（周日为1） */
#define CMOS_MON_DAY    0x7     /* CMOS一月中当前日：1–31 */
#define CMOS_CUR_MON    0x8     /* CMOS当前月：1–12 */
#define CMOS_CUR_YEAR   0x9     /* CMOS当前年：0–99 */
#define CMOS_DEV_TYPE   0x12    /* CMOS格式 */
#define CMOS_CUR_CEN    0x32    /* CMOS当前世纪：19–20? */

#define CMOS_REG_A_SR   0x0A    /* CMOS寄存器A状态 */
#define CMOS_REG_B_SR   0x0B    /* CMOS寄存器B状态 */

struct rtc_time
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint32_t year;
};

void init_rtc();
int get_update_in_progress_flag(void);
uint8_t get_rtc_register(int reg);
struct rtc_time *read_rtc(void);
unsigned long get_timestamp();
unsigned long to_timestamp(uint32_t year, uint32_t month, uint32_t day, uint32_t hour, uint32_t minute, uint32_t second);

#endif /* _RTC_H_ */
