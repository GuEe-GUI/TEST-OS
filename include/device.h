#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <kernel.h>

static inline void  __attribute__((noreturn)) poweroff()
{
    LOG("poweroff");

    /* QEMU (newer) */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0x604), "a"(0x2000));
    /* QEMU (2.0)， bochs */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0xb004), "a"(0x2000));

    PANIC("poweroff fail\n");
}

void init_keyboard(void);
uint8_t get_key();

#endif
