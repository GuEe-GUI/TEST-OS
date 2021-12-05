#include <io.h>
#include <interrupt.h>
#include <kernel.h>
#include <string.h>
#include <vbe.h>

struct idt idt[256];
struct idtr idtr;
interrupt_handler interrupt_handlers[256] = {NULL};

static void init_idt_descriptor(uint16_t segment_selector, uint32_t offset, uint16_t type, struct idt *idt)
{
    idt->offset0_15 = offset & 0xffff;
    idt->segment_selector = segment_selector;
    idt->type.all = type;
    idt->offset16_31 = (offset & 0xffff0000) >> 16;
}

void interrupt_register(uint8_t vector, interrupt_handler handler)
{
    interrupt_handlers[vector] = handler;
}

void interrupt_enable(uint8_t vector)
{
    static uint16_t ocw1 = 0xFF;

    vector -= IRQ_0;
    ocw1 &= (uint16_t)~((1 << vector));

    if (vector < 8)
    {
        io_out8(PIC0_IMR, (uint8_t)(ocw1 & 0xFF));
    }
    else
    {
        io_out8(PIC1_IMR, (uint8_t)(ocw1 >> 8));
    }
}

void interrupt_disable(uint8_t vector)
{
    static uint16_t ocw1 = 0xFF;

    vector -= IRQ_0;
    ocw1 |= (uint16_t)((1 << vector));

    if (vector < 8)
    {
        io_out8(PIC0_IMR, (uint8_t)(ocw1 & 0xFF));
    }
    else
    {
        io_out8(PIC1_IMR, (uint8_t)(ocw1 >> 8));
    }
}

void isr_handler(struct registers *registers)
{
    if (interrupt_handlers[registers->interrupt_vector] != NULL)
    {
        interrupt_handlers[registers->interrupt_vector](registers);
    }
    else
    {
        PANIC("unhook interrupt vector: %d\n", registers->interrupt_vector);
    }
}

void irq_handler(struct registers *registers)
{
    /* 发送重置信号 */
    if (registers->interrupt_vector >= 40)
    {
        /* 主 */
        io_out8(PIC1_ICW1, 0x20);
    }
    /* 从 */
    io_out8(PIC0_OCW2, 0x20);

    if (interrupt_handlers[registers->interrupt_vector] != NULL)
    {
        interrupt_handlers[registers->interrupt_vector](registers);
    }
}

void init_idt(void)
{
    memset((void *)interrupt_handlers, 0, sizeof(interrupt_handler));

    idtr.limite = (sizeof(struct idt) << 8) - 1;
    idtr.base  = (uint32_t)&idt;

    memset((void *)idt, 0, sizeof(idt));

    init_idt_descriptor(0x08, (uint32_t)isr0,  INT_GATE, &idt[0]);
    init_idt_descriptor(0x08, (uint32_t)isr1,  INT_GATE, &idt[1]);
    init_idt_descriptor(0x08, (uint32_t)isr2,  INT_GATE, &idt[2]);
    init_idt_descriptor(0x08, (uint32_t)isr3,  INT_GATE, &idt[3]);
    init_idt_descriptor(0x08, (uint32_t)isr4,  INT_GATE, &idt[4]);
    init_idt_descriptor(0x08, (uint32_t)isr5,  INT_GATE, &idt[5]);
    init_idt_descriptor(0x08, (uint32_t)isr6,  INT_GATE, &idt[6]);
    init_idt_descriptor(0x08, (uint32_t)isr7,  INT_GATE, &idt[7]);
    init_idt_descriptor(0x08, (uint32_t)isr8,  INT_GATE, &idt[8]);
    init_idt_descriptor(0x08, (uint32_t)isr9,  INT_GATE, &idt[9]);
    init_idt_descriptor(0x08, (uint32_t)isr10, INT_GATE, &idt[10]);
    init_idt_descriptor(0x08, (uint32_t)isr11, INT_GATE, &idt[11]);
    init_idt_descriptor(0x08, (uint32_t)isr12, INT_GATE, &idt[12]);
    init_idt_descriptor(0x08, (uint32_t)isr13, INT_GATE, &idt[13]);
    init_idt_descriptor(0x08, (uint32_t)isr14, INT_GATE, &idt[14]);
    init_idt_descriptor(0x08, (uint32_t)isr15, INT_GATE, &idt[15]);
    init_idt_descriptor(0x08, (uint32_t)isr16, INT_GATE, &idt[16]);
    init_idt_descriptor(0x08, (uint32_t)isr17, INT_GATE, &idt[17]);
    init_idt_descriptor(0x08, (uint32_t)isr18, INT_GATE, &idt[18]);
    init_idt_descriptor(0x08, (uint32_t)isr19, INT_GATE, &idt[19]);
    init_idt_descriptor(0x08, (uint32_t)isr20, INT_GATE, &idt[20]);
    init_idt_descriptor(0x08, (uint32_t)isr21, INT_GATE, &idt[21]);
    init_idt_descriptor(0x08, (uint32_t)isr22, INT_GATE, &idt[22]);
    init_idt_descriptor(0x08, (uint32_t)isr23, INT_GATE, &idt[23]);
    init_idt_descriptor(0x08, (uint32_t)isr24, INT_GATE, &idt[24]);
    init_idt_descriptor(0x08, (uint32_t)isr25, INT_GATE, &idt[25]);
    init_idt_descriptor(0x08, (uint32_t)isr26, INT_GATE, &idt[26]);
    init_idt_descriptor(0x08, (uint32_t)isr27, INT_GATE, &idt[27]);
    init_idt_descriptor(0x08, (uint32_t)isr28, INT_GATE, &idt[28]);
    init_idt_descriptor(0x08, (uint32_t)isr29, INT_GATE, &idt[29]);
    init_idt_descriptor(0x08, (uint32_t)isr30, INT_GATE, &idt[30]);
    init_idt_descriptor(0x08, (uint32_t)isr31, INT_GATE, &idt[31]);

    init_idt_descriptor(0x08, (uint32_t)irq0,  INT_GATE, &idt[32]);
    init_idt_descriptor(0x08, (uint32_t)irq1,  INT_GATE, &idt[33]);
    init_idt_descriptor(0x08, (uint32_t)irq2,  INT_GATE, &idt[34]);
    init_idt_descriptor(0x08, (uint32_t)irq3,  INT_GATE, &idt[35]);
    init_idt_descriptor(0x08, (uint32_t)irq4,  INT_GATE, &idt[36]);
    init_idt_descriptor(0x08, (uint32_t)irq5,  INT_GATE, &idt[37]);
    init_idt_descriptor(0x08, (uint32_t)irq6,  INT_GATE, &idt[38]);
    init_idt_descriptor(0x08, (uint32_t)irq7,  INT_GATE, &idt[39]);
    init_idt_descriptor(0x08, (uint32_t)irq8,  INT_GATE, &idt[40]);
    init_idt_descriptor(0x08, (uint32_t)irq9,  INT_GATE, &idt[41]);
    init_idt_descriptor(0x08, (uint32_t)irq10, INT_GATE, &idt[42]);
    init_idt_descriptor(0x08, (uint32_t)irq11, INT_GATE, &idt[43]);
    init_idt_descriptor(0x08, (uint32_t)irq12, INT_GATE, &idt[44]);
    init_idt_descriptor(0x08, (uint32_t)irq13, INT_GATE, &idt[45]);
    init_idt_descriptor(0x08, (uint32_t)irq14, INT_GATE, &idt[46]);
    init_idt_descriptor(0x08, (uint32_t)irq15, INT_GATE, &idt[47]);

    /* 加载IDTR */
    __asm__ volatile ("lidtl (idtr)");

    LOG("load idtr limite = %d, base = 0x%p\n", idtr.limite, idtr.base);
}

void init_8259a(void)
{
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

    io_out8(PIC0_IMR, 0xff);
    io_out8(PIC1_IMR, 0xff);

    LOG("interrupt control is 8259a\n");
}

void print_interrupt()
{
    int i, base, limite, len;
    const char *interrupt_type[] =
    {
        "fault", "trap", "fault/trap", "abort", "interrupt"
    };
    const int reserved = ARRAY_SIZE(interrupt_type);
    const struct
    {
        char *instructions;
        uint8_t type;
    } isr_info[] =
    {
        { "#DE divide zero", 0 },
        { "#DB debug", 2 },
        { "NMI non-maskable external interrupt", 4 },
        { "#BP breakpoint", 1 },
        { "#OF overflow", 1 },
        { "#BR bound range exceeded", 0 },
        { "#UD invalid opcode", 0 },
        { "#NM device not available", 0 },
        { "#DF double fault", 3 },
        { "#MF coprocessor segment overrun", 0 },
        { "#TS invalid tss", 0 },
        { "#NP segment not present", 0 },
        { "#SS stack fault", 0 },
        { "#GP general protection", 0 },
        { "#PF page-fault", 0 },
        { NULL, reserved }, /* intel reserved */
        { "#MF fpu floating-point error", 0 },
        { "#AC alignment check", 0 },
        { "#MC machine-check", 3 },
        { "#XM simd floating-point", 0 },
    }, irq_info[] =
    {
        { "cmos", 4 },
        { "ps2 keyboard", 4 },
        { "link IRQ9", 4 },
        { "serial 2", 4 },
        { "serial 1", 4 },
        { "parallel 2", 4 },
        { "floppy", 4 },
        { "parallel 1", 4 },
        { "rtc", 4 },
        { "redirect IRQ2", 4 },
        { NULL, reserved },
        { NULL, reserved },
        { "ps2 mouse", 4 },
        { "fpu exception", 4 },
        { "ide", 4 },
        { NULL, reserved },
    }, *info;

    set_color_invert();
    printk(" interrupt vector | type       | status | instructions                        ");
    set_color_invert();
    printk("\n");

    info = isr_info;
    base = 0;
    limite = ARRAY_SIZE(isr_info);

print_continue:
    for (i = 0; i < limite; ++i)
    {
        if (info[i].type == reserved)
        {
            continue;
        }
        len = printk(" %d", i + base);
        while (len < 18)
        {
            printk(" ");
            ++len;
        }
        printk("|");
        len = printk(" %s", interrupt_type[info[i].type]);
        while (len < 12)
        {
            printk(" ");
            ++len;
        }
        printk("|");
        len = printk(" %s", interrupt_handlers[i + base] == NULL ? "unused" : "using");
        while (len < 8)
        {
            printk(" ");
            ++len;
        }
        printk("| %s\n", info[i].instructions);
    }

    if (info != irq_info)
    {
        info = irq_info;
        base = IRQ_0;
        limite = ARRAY_SIZE(irq_info);
        goto print_continue;
    }
}
