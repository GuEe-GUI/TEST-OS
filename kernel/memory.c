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

    LOG("ards number = %d\n", ards_number);
    LOG("device total memory = %dMB\n", total_memory_bytes / (1 * MB));
}

static inline void memory_remap()
{
    int i, j;

    uint32_t map_vaddr;
    uint32_t map_paddr;
    uint32_t dir_limit;

    uint32_t *dir_paddr = (void *)KERNEL_PAGE_DIR_PADDR;
    uint32_t *tbl_paddr = (void *)KERNEL_VA_PAGE_PADDR;

    cli();

    /* 从内核只需要最多大约1G内存空间，映射的内存区域[0 ~ KERNEL_PAGE_MAP_MAX] */
    if (total_memory_bytes > KERNEL_PAGE_MAP_MAX)
    {
        total_memory_bytes = KERNEL_PAGE_MAP_MAX;
    }

    /* 从物理地址0x00000000开始映射 */
    map_paddr = 0x00000000;
    map_vaddr = KERNEL_MAP_BASE_VADDR;
    /* 页目录总共4k，可映射4G内存，则 / 4M */
    dir_limit = total_memory_bytes / (4 * MB);

    for (i = 0; i < dir_limit; ++i)
    {
        /* 页目录[31:22]，设置当前基地址对应的页表项 */
        dir_paddr[map_vaddr >> 22] = (uint32_t)tbl_paddr | KERNEL_PAGE_ATTR;

        /* 填写每个页表项，1024（4KB / sizeof(void*)）个entry */
        for (j = 0; j < 1024; ++j)
        {
            /* 页表[21:12]映射4K物理地址 */
            tbl_paddr[(map_vaddr >> 12) & 0x3ff] = map_paddr | KERNEL_PAGE_ATTR;
            map_paddr += PAGE_SIZE;
        }
        tbl_paddr += 1024;

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
