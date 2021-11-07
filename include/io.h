#ifndef _IO_H_
#define _IO_H_

#include <types.h>

#define cli() __asm__ volatile ("cli")
#define hlt() __asm__ volatile ("hlt")
#define sti() __asm__ volatile ("sti")
#define nop() __asm__ volatile ("nop")

extern void io_out8(uint32_t port, uint8_t data);
extern void io_out16(uint32_t port, uint16_t data);
extern void io_out32(uint32_t port, uint32_t data);
extern uint8_t io_in8(uint32_t port);
extern uint16_t io_in16(uint32_t port);
extern uint32_t io_in32(uint32_t port);

#endif
