#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <types.h>

#define KERNEL_MAP_BASE_VADDR   0x80000000
#define KERNEL_MAP_BASE_PADDR   0x200000
#define KERNEL_ENTRY_BASE_VADDR (KERNEL_MAP_BASE_VADDR + KERNEL_MAP_BASE_PADDR)

/* 0x500 是BIOS ROM */
#define KERNEL_STACK_SIZE       (4 * KB + (0x1000 - 0x500))
#define KERNEL_STACK_TOP        0x80002000

/* 内核小于1MB，其他内存设置在内核起点1MB后 */
#define KERNEL_USING_SIZE       (1 * MB)
#define KERNEL_USING_BASE       (KERNEL_ENTRY_BASE_VADDR + KERNEL_USING_SIZE)
/* 预留8M给内核使用，其他用于堆 */
#define KERNEL_HEAP_BOTTOM      KERNEL_USING_BASE

#define PAGE_SHIFT              12
#define PAGE_SIZE               (1U << PAGE_SHIFT)

#define PAGE_ATTR_NOT_PRESENT   0   /* 0000 不存在内存上 */
#define PAGE_ATTR_PRESENT       1   /* 0001 存在内存上 */
#define PAGE_ATTR_READ          0   /* 0000 R/W 可读/可执行 */
#define PAGE_ATTR_WRITE         2   /* 0010 R/W 可读/可写/可执行*/
#define PAGE_ATTR_SYSTEM        0   /* 0000 U/S 系统, cpl0,1,2 */
#define PAGE_ATTR_USER          4   /* 0100 U/S 用户, cpl3 */

#define KERNEL_PAGE_ATTR        (PAGE_ATTR_PRESENT | PAGE_ATTR_WRITE | PAGE_ATTR_SYSTEM)

#define KERNEL_PAGE_DIR_PADDR   0x3000
#define KERNEL_VBE_PAGE_PADDR   0x4000
#define KERNEL_PA_PAGE_PADDR    0x5000
#define KERNEL_VA_PAGE_PADDR    0x6000

#define KERNEL_PAGE_MAP_MAX     ((KERNEL_MAP_BASE_PADDR - KERNEL_VA_PAGE_PADDR) * PAGE_SIZE / sizeof(void *))

#define ARDS_N_ADDRESS          (KERNEL_MAP_BASE_VADDR + 0x7010)    /* ARDS的数量 */
#define ARDS_ADDRESS            (KERNEL_MAP_BASE_VADDR + 0x7050)    /* ARDS数据结构的内存地址 */

struct ards
{
    uint32_t base_addr_low;     /* 基地址低32位 */
    uint32_t base_addr_high;    /* 基地址高32位 */
    uint32_t length_low;        /* 内存长度低32位，以字节为单位 */
    uint32_t length_high;       /* 内存长度高32位，以字节为单位 */
    uint32_t type;              /* 内存类型 */
};

void init_memory();
uint32_t get_total_memory_bytes();

#endif
