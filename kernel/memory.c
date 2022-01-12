#include <memory.h>
#include <interrupt.h>
#include <kernel.h>
#include <string.h>

static uint32_t total_memory_bytes = 0;

static inline void init_ards()
{
    struct ards *ards;
    uint16_t ards_number = *(int16_t *)ARDS_N_ADDRESS;
    int number = ards_number, sum;

    ards = (struct ards *)ARDS_ADDRESS;

    for (; number >= 0; --number)
    {
        /* 判断类型是否是可以被操作系统使用（address_range_memory）*/
        if (ards->type == 1)
        {
            sum = ards->base_addr_low + ards->length_low;
            /* 获取最大内存量 */
            if (sum > total_memory_bytes)
            {
                total_memory_bytes = sum;
            }
        }
        ++ards;
    }

    total_memory_bytes += 1 * MB;

    LOG("ards number = %d", ards_number);
    LOG("device total memory = %dMB", total_memory_bytes / (1 * MB));
}

static inline void memory_remap()
{
    int i;
    uint32_t map_vaddr = KERNEL_MAP_BASE_VADDR + KERNEL_MAP_EARLY_SIZE;
    uint32_t map_paddr = KERNEL_MAP_BASE_PADDR + KERNEL_MAP_EARLY_SIZE;
    uint32_t *dir_paddr = (uint32_t *)KERNEL_PAGE_DIR_PADDR;
    uint32_t *tbl_paddr = (uint32_t *)KERNEL_REMAP_PAGE_PADDR;

    /* 内核只需要最多大约1G内存空间，映射的内存区域[0 ~ KERNEL_MAP_ADDR_MAX] */
    if (total_memory_bytes > KERNEL_MAP_ADDR_MAX)
    {
        total_memory_bytes = KERNEL_MAP_ADDR_MAX;
    }

    /* 页目录总共4k，可映射4G内存，则 / 4M */
    while (map_paddr <= total_memory_bytes)
    {
        /* 页目录[31:22]，设置当前基地址对应的页表项 */
        dir_paddr[map_vaddr >> 22] = (uint32_t)tbl_paddr | KERNEL_PAGE_ATTR;

        /* 填写每个页表项，1024（4KB / sizeof(void *)）个entry */
        for (i = 0; i < 1024 && map_paddr <= total_memory_bytes; ++i)
        {
            /* 页表[21:12]，映射4K物理地址 */
            tbl_paddr[(map_vaddr >> 12) & 0x3ff] = map_paddr | KERNEL_PAGE_ATTR;

            map_vaddr += PAGE_SIZE;
            map_paddr += PAGE_SIZE;
        }
        tbl_paddr += 1024;
    }

    /* 前4K给空指针NULL预留，设置属性为无效内存 */
    ((uint32_t *)KERNEL_PA_PAGE_PADDR)[0] = 0 | PAGE_ATTR_NOT_PRESENT;
    ((uint32_t *)KERNEL_VA_PAGE_PADDR)[0] = 0 | PAGE_ATTR_NOT_PRESENT;

    LOG("remap physical memory range = <%dMB %dMB>", (KERNEL_MAP_BASE_PADDR + KERNEL_MAP_EARLY_SIZE) / (1 * MB), total_memory_bytes / (1 * MB));
    LOG("remap virtual memory range = <0x%p 0x%p>", KERNEL_MAP_BASE_VADDR + KERNEL_MAP_EARLY_SIZE, map_paddr - PAGE_SIZE);
}

void __attribute__((noreturn)) page_failure_isr(struct registers *regs)
{
    set_color(0x8250dfff, CONSOLE_CLEAR);
    printk("page failure error\n");
    print_registers(regs);
    ASSERT(0);
}

void init_memory(void)
{
    init_ards();
    memory_remap();

    interrupt_register(14, page_failure_isr);

    LOG("kernel stack top addr = 0x%p, size = %dB", KERNEL_STACK_TOP, KERNEL_STACK_SIZE);
    LOG("memory hook page failure");
}

uint32_t get_total_memory_bytes(void)
{
    return total_memory_bytes;
}
