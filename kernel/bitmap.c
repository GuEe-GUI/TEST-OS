#include <bitmap.h>
#include <io.h>
#include <kernel.h>
#include <memory.h>
#include <string.h>

static struct bitmap bitmap;

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

    LOG("bitmap range = <0x%p 0x%p>, size = %dMB\n", bitmap.bits_start, bitmap.bits_end, bytes_len / (1 * MB));
}

void *bitmap_malloc(size_t size)
{
    int i, j;
    size_t found;
    uint32_t len;
    uint8_t *ret;

    cli();

    i = size;
    size /= (4 * KB);
    if (i % (4 * KB) != 0)
    {
        size += 1;
    }

    /* 记录找到的4KB的个数 */
    found = 0;
    ret = NULL;
    /* 获取当前bitmap内存指针 */
    i = (bitmap.bits_ptr - bitmap.bits_start) / (8 * 4 * KB);
    /* 作为i的限制 */
    len = bitmap.map_size;

found_prev:
    for (; i < len; ++i)
    {
        /* 查找每个byte中的位 */
        for (j = 0; j < 8; ++j)
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
                /* 标记内存大小（内存至少分配4KB，用1byte记录内存大小影响不大） */
                *ret++ = (uint8_t)size;
                /* 地址+1返回 */
                bitmap.bits_ptr = (uint32_t)ret + alloc_size;
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

    sti();

    return ret;
}

void bitmap_free(void *addr)
{
    size_t size;
    int i, j;

    cli();

    /* size存放在前一个字节，前一个字节开始为申请内存地址的起点 */
    addr = ((uint8_t *)addr) - 1;
    size = *((uint8_t *)addr);
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
    sti();
}

void print_mem()
{
    size_t total = bitmap.map_size;
    size_t free = 0;
    size_t used = 0;
    int i, j;

    cli();

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

    total = total * (4 * KB * 8) / KB;
    used  = used  * (4 * KB) / KB;
    free  = free  * (4 * KB) / KB;

    printk("mem (KB): %d total, %d free, %d used\n", total, free, used);

    sti();
}
