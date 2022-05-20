#include <fpu.h>
#include <interrupt.h>
#include <kernel.h>
#include <types.h>

static void __attribute__((noreturn)) fpu_isr(struct registers *regs)
{
    PANIC("fpu floating-point error");
}

void init_fpu()
{
    uint32_t reg_val;

    __asm__ volatile ("clts");
    __asm__ volatile ("mov %%cr0, %0":"=r"(reg_val));

    reg_val &= ~(1 << 2);
    reg_val |= (1 << 1);

    __asm__ volatile ("mov %0, %%cr0"::"r"(reg_val));
    __asm__ volatile ("mov %%cr4, %0":"=r"(reg_val));

    reg_val |= 3 << 9;

    __asm__ volatile ("mov %0, %%cr4"::"r"(reg_val));
    __asm__ volatile ("fninit");

    interrupt_register(16, fpu_isr);

    LOG("fpu hook floating-point error trap");
}
