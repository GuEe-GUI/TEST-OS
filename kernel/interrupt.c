#include <io.h>
#include <interrupt.h>
#include <kernel.h>
#include <string.h>

struct idt idt[256];
struct idtr idtr;
interrupt_handler interrupthandlers[256] = {NULL};

static void init_idt_descriptor(uint16_t segment_selector, uint32_t offset, uint16_t type, struct idt *idt)
{
    idt->offset0_15 = offset & 0xffff;
    idt->segment_selector = segment_selector;
    idt->type.all = type;
    idt->offset16_31 = (offset & 0xffff0000) >> 16;
    return;
}

void register_interrupt(uint8_t number, interrupt_handler handler)
{
    cli();
    interrupthandlers[number] = handler;

    uint16_t port = number < 8 ? PIC0_IMR : PIC1_IMR;
    uint8_t value = io_in8(port) & ~(1 << number);
    io_out8(port, value);

    sti();
    return;
}

void isr_handler(struct registers *registers)
{
    if (interrupthandlers[registers->interrupt_number] != NULL)
    {
        interrupthandlers[registers->interrupt_number](registers);
    }
    else
    {
        PANIC("interrupt_number: %d\n", registers->interrupt_number);
    }
}

void irq_handler(struct registers *registers)
{
    /* 发送重置信号 */
    if (registers->interrupt_number >= 40)
    {
        /* 主 */
        io_out8(PIC1_ICW1, 0x20);
    }
    /* 从 */
    io_out8(PIC0_OCW2, 0x20);

    if (interrupthandlers[registers->interrupt_number] != NULL)
    {
        interrupthandlers[registers->interrupt_number](registers);
    }
}

void init_idt(void)
{
    memset((uint8_t*)&interrupthandlers, 0, sizeof(interrupt_handler) << 8);

    idtr.limite = (sizeof(struct idt) << 8) - 1;
    idtr.base  = (uint32_t)&idt;

    memset((uint8_t*)&idt, 0, sizeof(struct idt) << 8);

    init_idt_descriptor(0x08, (uint32_t)ISR0,  INT_GATE, &idt[0]);
    init_idt_descriptor(0x08, (uint32_t)ISR1,  INT_GATE, &idt[1]);
    init_idt_descriptor(0x08, (uint32_t)ISR2,  INT_GATE, &idt[2]);
    init_idt_descriptor(0x08, (uint32_t)ISR3,  INT_GATE, &idt[3]);
    init_idt_descriptor(0x08, (uint32_t)ISR4,  INT_GATE, &idt[4]);
    init_idt_descriptor(0x08, (uint32_t)ISR5,  INT_GATE, &idt[5]);
    init_idt_descriptor(0x08, (uint32_t)ISR6,  INT_GATE, &idt[6]);
    init_idt_descriptor(0x08, (uint32_t)ISR7,  INT_GATE, &idt[7]);
    init_idt_descriptor(0x08, (uint32_t)ISR8,  INT_GATE, &idt[8]);
    init_idt_descriptor(0x08, (uint32_t)ISR9,  INT_GATE, &idt[9]);
    init_idt_descriptor(0x08, (uint32_t)ISR10, INT_GATE, &idt[10]);
    init_idt_descriptor(0x08, (uint32_t)ISR11, INT_GATE, &idt[11]);
    init_idt_descriptor(0x08, (uint32_t)ISR12, INT_GATE, &idt[12]);
    init_idt_descriptor(0x08, (uint32_t)ISR13, INT_GATE, &idt[13]);
    init_idt_descriptor(0x08, (uint32_t)ISR14, INT_GATE, &idt[14]);
    init_idt_descriptor(0x08, (uint32_t)ISR15, INT_GATE, &idt[15]);
    init_idt_descriptor(0x08, (uint32_t)ISR16, INT_GATE, &idt[16]);
    init_idt_descriptor(0x08, (uint32_t)ISR17, INT_GATE, &idt[17]);
    init_idt_descriptor(0x08, (uint32_t)ISR18, INT_GATE, &idt[18]);
    init_idt_descriptor(0x08, (uint32_t)ISR19, INT_GATE, &idt[19]);
    init_idt_descriptor(0x08, (uint32_t)ISR20, INT_GATE, &idt[20]);
    init_idt_descriptor(0x08, (uint32_t)ISR21, INT_GATE, &idt[21]);
    init_idt_descriptor(0x08, (uint32_t)ISR22, INT_GATE, &idt[22]);
    init_idt_descriptor(0x08, (uint32_t)ISR23, INT_GATE, &idt[23]);
    init_idt_descriptor(0x08, (uint32_t)ISR24, INT_GATE, &idt[24]);
    init_idt_descriptor(0x08, (uint32_t)ISR25, INT_GATE, &idt[25]);
    init_idt_descriptor(0x08, (uint32_t)ISR26, INT_GATE, &idt[26]);
    init_idt_descriptor(0x08, (uint32_t)ISR27, INT_GATE, &idt[27]);
    init_idt_descriptor(0x08, (uint32_t)ISR28, INT_GATE, &idt[28]);
    init_idt_descriptor(0x08, (uint32_t)ISR29, INT_GATE, &idt[29]);
    init_idt_descriptor(0x08, (uint32_t)ISR30, INT_GATE, &idt[30]);
    init_idt_descriptor(0x08, (uint32_t)ISR31, INT_GATE, &idt[31]);

    init_idt_descriptor(0x08, (uint32_t)IRQ0,  INT_GATE, &idt[32]);
    init_idt_descriptor(0x08, (uint32_t)IRQ1,  INT_GATE, &idt[33]);
    init_idt_descriptor(0x08, (uint32_t)IRQ2,  INT_GATE, &idt[34]);
    init_idt_descriptor(0x08, (uint32_t)IRQ3,  INT_GATE, &idt[35]);
    init_idt_descriptor(0x08, (uint32_t)IRQ4,  INT_GATE, &idt[36]);
    init_idt_descriptor(0x08, (uint32_t)IRQ5,  INT_GATE, &idt[37]);
    init_idt_descriptor(0x08, (uint32_t)IRQ6,  INT_GATE, &idt[38]);
    init_idt_descriptor(0x08, (uint32_t)IRQ7,  INT_GATE, &idt[39]);
    init_idt_descriptor(0x08, (uint32_t)IRQ8,  INT_GATE, &idt[40]);
    init_idt_descriptor(0x08, (uint32_t)IRQ9,  INT_GATE, &idt[41]);
    init_idt_descriptor(0x08, (uint32_t)IRQ10, INT_GATE, &idt[42]);
    init_idt_descriptor(0x08, (uint32_t)IRQ11, INT_GATE, &idt[43]);
    init_idt_descriptor(0x08, (uint32_t)IRQ12, INT_GATE, &idt[44]);
    init_idt_descriptor(0x08, (uint32_t)IRQ13, INT_GATE, &idt[45]);
    init_idt_descriptor(0x08, (uint32_t)IRQ14, INT_GATE, &idt[46]);
    init_idt_descriptor(0x08, (uint32_t)IRQ15, INT_GATE, &idt[47]);

    /* 加载IDTR */
    __asm__ volatile ("lidtl (idtr)");

    LOG("load idtr limite = %d, base = %p\n", idtr.limite, idtr.base);
}

void init_8259a(void)
{
    uint8_t pic0_mask = io_in8(PIC0_IMR);
    uint8_t pic1_mask = io_in8(PIC1_IMR);

    io_out8(PIC0_IMR, 0xff);
    io_out8(PIC1_IMR, 0xff);

    io_out8(PIC0_ICW1, 0x11); /* 0001 0001 */
    io_out8(PIC0_ICW2, 0x20); /* 0010 0000 */
    io_out8(PIC0_ICW3, 0x04); /* 0000 0100 */
    io_out8(PIC0_ICW4, 0x01); /* 0000 0001 */

    io_out8(PIC1_ICW1, 0x11); /* 0001 0001 */
    io_out8(PIC1_ICW2, 0x28); /* 0010 1000 */
    io_out8(PIC1_ICW3, 0x02); /* 0000 0010 */
    io_out8(PIC1_ICW4, 0x01); /* 0000 0001 */

    io_out8(PIC0_IMR, pic0_mask);
    io_out8(PIC1_IMR, pic1_mask);

    LOG("interrupt control is 8259a\n");
}
