#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <types.h>

#define KERNEL_MAP_BASE_VADDR   0x80000000
#define KERNEL_MAP_BASE_PADDR   0x00000000

/* 内核小于1MB，其他内存设置在内核起点1MB后 */
#define KERNEL_IMAGE_BASE_PADDR 0x100000
#define KERNEL_IMAGE_BASE_VADDR (KERNEL_MAP_BASE_VADDR + KERNEL_IMAGE_BASE_PADDR)
#define KERNEL_IMAGE_SIZE       (1 * MB)
#define KERNEL_IMAGE_END_PADDR  (KERNEL_IMAGE_BASE_PADDR + KERNEL_IMAGE_SIZE)
#define KERNEL_IMAGE_END_VADDR  (KERNEL_IMAGE_BASE_VADDR + KERNEL_IMAGE_SIZE)

#define PAGE_SHIFT              12
#define PAGE_SIZE               (1U << PAGE_SHIFT)

#define PAGE_ATTR_NOT_PRESENT   0   /* 0000 不存在内存上 */
#define PAGE_ATTR_PRESENT       1   /* 0001 存在内存上 */
#define PAGE_ATTR_READ          0   /* 0000 R/W 可读/可执行 */
#define PAGE_ATTR_WRITE         2   /* 0010 R/W 可读/可写/可执行*/
#define PAGE_ATTR_SYSTEM        0   /* 0000 U/S 系统, cpl0,1,2 */
#define PAGE_ATTR_USER          4   /* 0100 U/S 用户, cpl3 */

#define KERNEL_PAGE_ATTR        (PAGE_ATTR_PRESENT | PAGE_ATTR_WRITE | PAGE_ATTR_SYSTEM)

#define KERNEL_PAGE_DIR_PADDR   0x3000  /* 页目录物理地址 */
#define KERNEL_VBE_PAGE_PADDR   0x4000  /* 显存页目录物理地址 */
#define KERNEL_PA_PAGE_PADDR    0x5000  /* Loader映射的物理内存页目录物理地址 */
#define KERNEL_VA_PAGE_PADDR    0x6000  /* Loader映射的虚拟内存页目录物理地址 */
#define KERNEL_REMAP_PAGE_PADDR KERNEL_IMAGE_END_PADDR

/* 内核保留的映射区域起始地址，可能用于ACPI，DMA等非普通内存的页表 */
#define KERNEL_MAP_RES_PADDR    (KERNEL_VA_PAGE_PADDR + PAGE_SIZE)
#define KERNEL_MAP_RES_SIZE     (KERNEL_IMAGE_BASE_PADDR - KERNEL_MAP_RES_PADDR)

#define KERNEL_MAP_EARLY_SIZE   (4 * MB)
#define KERNEL_MAP_ADDR_MAX     ((KERNEL_MAP_EARLY_SIZE - KERNEL_IMAGE_END_PADDR) / sizeof(void *) * PAGE_SIZE)

/* 0x500 是BIOS ROM */
#define KERNEL_STACK_SIZE       (4 * KB + (0x1000 - 0x500))
#define KERNEL_STACK_TOP        0x80002000

/* 估计需要预留内存给内核使用，其他用于堆 */
#define KERNEL_HEAP_BOTTOM      (KERNEL_IMAGE_END_VADDR + (KERNEL_MAP_EARLY_SIZE - KERNEL_IMAGE_END_PADDR))

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

void init_memory(void);
uint32_t get_total_memory_bytes(void);

#endif /* _MEMORY_H_ */
