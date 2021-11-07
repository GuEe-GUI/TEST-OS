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
        if (ards->type == 1) {
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

    LOG("ards number = %d\n", ards_number);
    LOG("device total memory = %dMB\n", total_memory_bytes / (1 * MB));

    return;
}

static inline void memory_remap()
{
    int i, j;

    uint32_t map_vaddr;
    uint32_t map_paddr;
    uint32_t dir_limit;

    uint32_t dir_paddr = KERNEL_PAGE_DIR_PADDR;
    uint32_t tbl1_paddr = KERNEL_PAGE_TBL1_PADDR;

    cli();

    /* 内核只需要最多1G内存空间，内核启动时已经映射了4M，则新映射的内存区域 > 4M, < total_memory_bytes */
    if (total_memory_bytes > 1 * GB)
    {
        total_memory_bytes = 1 * GB;
    }

    /* 继续映射高地址后面的地址 */
    map_vaddr = KERNEL_MAP_BASE_VADDR + 4 * MB;
    /* 从物理地址0x00000000开始映射到高地址，+4M 继续往后映射 */
    map_paddr = (0x00000000 + 4 * MB) | KERNEL_PAGE_ATTR;
    /* 页目录总共4k，可映射4G内存，则 / 4M，内核设定了限制，因此向上取整 */
    dir_limit = (total_memory_bytes - 4 * MB) / (4 * MB) + 1;

    for (i = 0; i < dir_limit; ++i)
    {
        /*
         * 一个页目录4k，对应地址4G
         * 0x80000000 >> 22 = 512, 512 * 4 = 2048 => 4G / 2
         */
        uint32_t map_off = (map_vaddr >> 22) * 4;

        /* 设置当前基地址对应的页表项 */
        *(uint32_t *)(dir_paddr + map_off) = tbl1_paddr | KERNEL_PAGE_ATTR;

        /* 填写每个页表项，每个1K（10bits） */
        for (j = 0; j < 1 * KB; ++j)
        {
            /* 每个页表映射4K（12bits）物理地址 */
            *(uint32_t *)tbl1_paddr = map_paddr | KERNEL_PAGE_ATTR;
            map_paddr += PAGE_SIZE;
            tbl1_paddr += sizeof(uint32_t);
        }

        /* 准备映射下一个4M的地址 */
        map_vaddr += 4 * MB;
    }

    sti();

    LOG("new map physical memory range = <4MB %dMB>\n", total_memory_bytes / (1 * MB));
    LOG("new map virtual memory range = <%p %p>\n", KERNEL_MAP_BASE_VADDR + 4 * MB, KERNEL_MAP_BASE_VADDR + total_memory_bytes);
}

void __attribute__((noreturn)) page_failure_isr(struct registers *reg)
{
    PANIC("page failure error in eip = %p, esp = %p\n", reg->eip, reg->esp);
}

void init_memory()
{
    init_ards();
    memory_remap();

    register_interrupt(14, page_failure_isr);

    LOG("kernel stack top addr = %p\n", KERNEL_STACK_TOP);
    LOG("memory hook page failure\n");
}

uint32_t get_total_memory_bytes()
{
    return total_memory_bytes;
}
