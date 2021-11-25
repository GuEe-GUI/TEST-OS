#ifndef _TIMER_H_
#define _TIMER_H_

#include <types.h>

#define CLOCK_TICK_RATE 1193180
#define KERNEL_TICK_HZ  100

void init_timer();
void delay(uint32_t millisecond);
void sleep(uint32_t millisecond);
void sleeper_polling();
void print_tick(void);

#endif
