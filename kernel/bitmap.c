#include <bitmap.h>
#include <io.h>
#include <kernel.h>
#include <memory.h>
#include <string.h>

#define BITMAP_HEAP_MALLOC_MAGIC    0x50414548  /* "HEAP" */
#define BITMAP_HEAP_FREE_MAGIC      0x70616568  /* "heap" */

static struct bitmap bitmap;
static pe_lock_t bitmap_lock = 0;

void init_bitmap(uint32_t bytes_len)
{
    /* 内核预留内存 */
    bytes_len -= KERNEL_HEAP_BOTTOM - KERNEL_MAP_BASE_VADDR;
    /*
     * 根据bytes_len分配一定的位图，每一位对应4KB
     * 则解方程：
     *      y = bitmap.map占用总空间（byte）
     *      x = bytes_len，堆起点后可使用的内存（byte）
     * =>   y * 8 * 4KB = x - y
     * =>   y = x / (8 * 4KB + 1)
     */
    bitmap.map_size = bytes_len / (8 * (4 * KB) + 1);
    bytes_len -= bitmap.map_size;
    bitmap.bits_start = KERNEL_HEAP_BOTTOM + bitmap.map_size;

    bitmap.map = (uint8_t *)KERNEL_HEAP_BOTTOM;
    bitmap.bits_ptr = bitmap.bits_start;
    bitmap.bits_end = bitmap.bits_start + bytes_len;

    memset(bitmap.map, 0, bitmap.map_size);

    LOG("bitmap range = <0x%p 0x%p>, size = %dMB", bitmap.bits_start, bitmap.bits_end, bytes_len / (1 * MB));
}

void *bitmap_malloc(size_t size)
{
    int i, j;
    size_t found;
    uint32_t len;
    uint8_t *ret;

    pe_lock(&bitmap_lock);

    /* 原则上参数为0默认申请一个字节 */
    if (size == 0)
    {
        size = 1;
    }

    /* 我们需要8字节分别标记堆标识和堆大小 */
    size += 8;

    /* 计算需要4K的数量 */
    i = size;
    size /= (4 * KB);
    if (i % (4 * KB) != 0)
    {
        size += 1;
    }

    /* 记录找到的4KB的个数 */
    found = 0;
    ret = NULL;
    /* 获取当前bitmap内存指针来计算i和j，i为map的索引，j为byte中的bit索引 */
    i = (bitmap.bits_ptr - bitmap.bits_start) / (8 * 4 * KB);
    j = (bitmap.bits_ptr - bitmap.bits_start) / (4 * KB) - (i * 8);
    /* 作为i的限制 */
    len = bitmap.map_size;

found_prev:
    for (; i < len; ++i)
    {
        /* 查找每个byte中的位 */
        for (; j < 8; ++j)
        {
            if (((bitmap.map[i] >> j) & 1) == 0)
            {
                /* 如果找到+1 */
                found++;
            }
            else
            {
                /* 如果是1则不能连续，设为0 */
                found = 0;
            }
            if (found == size)
            {
                size_t alloc_size = size * (4 * KB);
                ret = (void *)(bitmap.bits_start + (i * 8 + j + 1) * (4 * KB) - alloc_size);
                /* 标记内存为堆 */
                *(uint32_t *)ret = BITMAP_HEAP_MALLOC_MAGIC;
                ret += 4;
                /* 标记内存大小 */
                *(uint32_t *)ret = size;
                /* 此时返回实际堆地址 */
                ret += 4;
                /* 记录下一次可能分配的地址 */
                bitmap.bits_ptr = (uint32_t)ret - 8 + alloc_size;
                /* 标记位图被占用 */
                while (size > 0)
                {
                    bitmap.map[i] |= 1 << j;
                    --j;
                    --size;
                    if (j < 0)
                    {
                        j = 7;
                        --i;
                    }
                }
                goto end;
            }
        }
        j = 0;
    }

end:
    /* 如果查找失败就重头查找 */
    if (ret == NULL && len == bitmap.map_size)
    {
        len = bitmap.map_size - (bitmap.bits_end - bitmap.bits_ptr) / (8 * 4 * KB) - 1;
        i = 0;
        bitmap.bits_ptr = bitmap.bits_start;

        goto found_prev;
    }
    /* 如果重头查找也失败就直接返回 */

    pe_unlock(&bitmap_lock);

    return ret;
}

void *bitmap_realloc(void *addr, size_t size)
{
    /* 这里不再基于bitmap处理，而是直接用新内存（size存放在addr - 4字节） */
    size_t old_size = *(((uint8_t *)addr) - 4) * 4 * KB;

    if (old_size < size)
    {
        void *new_ptr = bitmap_malloc(size);

        if (new_ptr != NULL)
        {
            memcpy(new_ptr, addr, old_size);
            bitmap_free(addr);
        }

        return new_ptr;
    }

    return addr;
}

void bitmap_free(void *addr)
{
    size_t size;
    int i, j;

    pe_lock(&bitmap_lock);

    /* size存放在addr - 4字节，堆标记在addr - 8字节 */
    addr = ((uint8_t *)addr) - 8;
    ASSERT(*(uint32_t *)addr == BITMAP_HEAP_MALLOC_MAGIC);
    *(uint32_t *)addr = BITMAP_HEAP_FREE_MAGIC;
    size = *((uint32_t *)(((uint8_t *)addr) + 4));

    /* 在bitmap哪个地址 */
    i = ((uint32_t)addr - bitmap.bits_start) / (8 * 4 * KB);
    /* 在该地址哪个bit */
    j = (((uint32_t)addr - bitmap.bits_start) - i * (4 * KB * 8)) / (4 * KB);

    for (; i < bitmap.map_size; ++i)
    {
        for (; j < 8; ++j)
        {
            /* 设置该位为0 */
            bitmap.map[i] &= ~(1 << j);
            --size;
            if (size == 0)
            {
                goto end;
            }
        }
        j = 0;
    }

end:
    pe_unlock(&bitmap_lock);
}

void print_mem(void)
{
    size_t total = bitmap.map_size;
    size_t free = 0;
    size_t used = 0;
    int i, j;

    pe_lock(&bitmap_lock);

    for (i = total - 1; i >= 0; --i)
    {
        for (j = 0; j < 8; ++j)
        {
            if (((bitmap.map[i] >> j) & 1) == 0)
            {
                ++free;
            }
            else
            {
                ++used;
            }
        }
    }

    pe_unlock(&bitmap_lock);

    total = total * (4 * KB * 8) / KB;
    used  = used  * (4 * KB) / KB;
    free  = free  * (4 * KB) / KB;

    printk("mem (KB): %d total, %d free, %d used\n", total, free, used);
}
