#include <io.h>
#include <rtc.h>
#include <thread.h>

void pe_lock(pe_lock_t *lock)
{
    int eflags;
    bool locked = false;

    for (;;)
    {
        cli_inherit(&eflags);

        if (!*lock)
        {
            /* 原子操作，上锁 */
            __asm__ volatile ("lock orl $1, %1":"=m"(*lock):"m"(*lock));
            locked = true;
        }

        sti_inherit(&eflags);

        if (locked)
        {
            break;
        }
        else
        {
            thread_yield();
        }
    }
}

void pe_unlock(pe_lock_t *lock)
{
    /* 原子操作，解锁 */
    __asm__ volatile ("lock andl $0, %1":"=m"(*lock):"m"(*lock));
}

uint32_t soft_rand()
{
    static uint32_t next = 1;

    next = next * 1103515245 + 12345;

    return (uint32_t)(next / 65536) % 32768;
}

uint32_t hard_rand()
{
    return soft_rand() * (get_timestamp() % 10);
}
