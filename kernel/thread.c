#include <io.h>
#include <kernel.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <thread.h>
#include <vbe.h>

#define TID_BIT_NR  (sizeof(tid_t) * 8)

static struct thread *thread_head[THREAD_STATUS_SIZE] = {NULL};
static struct thread *current_thread = NULL, *fix_next_thread = NULL;
static tid_t tid_bitmap[KERNEL_THREAD_MAX / TID_BIT_NR] = {1}; /* 0号tid为缺省值 */

extern void switch_to_next_thread(void *prev, void *next);

static int thread_find_by_tid(tid_t tid, enum THREAD_STATUS status, struct thread** prev, struct thread **node)
{
    int result = -1;

    *prev = NULL;
    *node = thread_head[status];

    while (*node != NULL)
    {
        if ((*node)->tid == tid)
        {
            result = 0;
            break;
        }
        *prev = *node;
        *node = (*node)->next;
    }

    return result;
}

static void __attribute__((noreturn)) thread_died()
{
    cli();

    thread_exit(current_thread->tid);

    sti();

    /* 等待被销毁 */
    for (;;);
}

void __attribute__((noreturn)) start_thread(void *thread_list)
{
    tid_t tid;
    struct thread *thread_cleaner = (struct thread *)malloc(KERNEL_THREAD_STACK_SIZE);

    if ((thread_head[THREAD_READY] = thread_cleaner) == NULL)
    {
        PANIC("init thread cleaner fail\n");
    }

    thread_cleaner->tid = 0;
    thread_cleaner->wake_millisecond = 0;
    thread_cleaner->handler = &&thread_cleaner_entry;
    thread_cleaner->params = NULL;
    memcpy(thread_cleaner->name, "thread cleaner", sizeof("thread cleaner"));

    thread_wake(thread_cleaner->tid);
    LOG("thread cleaner handler = %p, tid = %d\n", thread_cleaner->handler, thread_cleaner->tid);

    if (thread_list != NULL)
    {
        int i = 0;
        while (PTR_LIST_ITEM(thread_list, i) != NULL)
        {
            char *name = (char *)PTR_LIST_ITEM(PTR_LIST_ITEM(thread_list, i), 0);
            void *handler = (char *)PTR_LIST_ITEM(PTR_LIST_ITEM(thread_list, i), 1);
            void *params = (char *)PTR_LIST_ITEM(PTR_LIST_ITEM(thread_list, i), 2);

            ++i;

            if (thread_create(&tid, name, handler, params))
            {
                PANIC("create %s thread fail\n", name);
            }

            thread_wake(tid);
            LOG("%s thread handler = %p, tid = %d\n", name, handler, tid);
        }
    }

    cli();
    /* 允许线程调度，第一个执行的线程为thread_cleaner */
    current_thread = thread_cleaner;

    /* 主动恢复thread_cleaner的esp */
    __asm__ volatile ("movl %0, %%esp"::"r"(thread_cleaner->context.esp));

thread_cleaner_entry:
    sti();

    for (;;)
    {
        struct thread *prev = NULL;
        struct thread *node = thread_head[THREAD_DIED];

        while (node != NULL)
        {
            if (node->ref == 0)
            {
                cli();

                if (prev != NULL)
                {
                    prev->next = node->next;
                }
                else
                {
                    /* node节点为thread_head[THREAD_DIED] */
                    thread_head[THREAD_DIED] = node->next;
                }
                /* 归还tid */
                tid_bitmap[node->tid / TID_BIT_NR] &= ~(1 << (node->tid % TID_BIT_NR));
                free(node);

                sti();
            }
            prev = node;
            node = node->next;
        }
        /* 可销毁节点已空，让出cpu使用权 */
        thread_yield();
    }
}

void thread_schedule()
{
    cli();

    if (current_thread != NULL)
    {
        struct thread *prev = current_thread;

        if (fix_next_thread != NULL)
        {
            current_thread = fix_next_thread;
            fix_next_thread = NULL;
        }
        else if (current_thread->next != NULL)
        {
            current_thread = current_thread->next;
        }
        else
        {
            current_thread = thread_head[THREAD_RUNNING];
        }

        /* 中断将会在此重新打开 */
        switch_to_next_thread(&prev->context, &current_thread->context);
    }

    sti();
}

int thread_create(tid_t *tid, char *name, void *handler, void *params)
{
    int map_idx, bit_idx, status = -1;
    struct thread *new_thread = (struct thread *)malloc(KERNEL_THREAD_STACK_SIZE);

    if (new_thread != NULL)
    {
        cli();

        memset(new_thread, 0, KERNEL_THREAD_STACK_SIZE);

        for (map_idx = 0; map_idx < sizeof(tid_bitmap) / sizeof(tid_bitmap[0]); ++map_idx)
        {
            for (bit_idx = 0; bit_idx < (sizeof(tid_t) * 8); ++bit_idx)
            {
                if (((tid_bitmap[map_idx] >> bit_idx) & 1) == 0)
                {
                    tid_bitmap[map_idx] |= 1 << bit_idx;

                    *tid = map_idx * (sizeof(tid_t) * 8) + bit_idx;
                    new_thread->tid = *tid;
                    new_thread->status = THREAD_READY;
                    new_thread->wake_millisecond = 0;
                    new_thread->ref = 0;
                    new_thread->handler = handler;
                    new_thread->params = params;
                    new_thread->context.esp = 0;

                    if (name == NULL)
                    {
                        name = "unnamed";
                    }
                    memset(new_thread->name, 0, KERNEL_THREAD_NAME_LEN);
                    memcpy(new_thread->name, name, strlen(name));
                    new_thread->name[KERNEL_THREAD_NAME_LEN - 1] = '\0';

                    if (thread_head[THREAD_READY] != NULL)
                    {
                        new_thread->next = thread_head[THREAD_READY];
                    }
                    thread_head[THREAD_READY] = new_thread;

                    status = 0;

                    goto finsh;
                }
            }
        }

        free(new_thread);

finsh:
        sti();
    }

    return status;
}

struct thread *thread_current()
{
    ASSERT(current_thread);

    return current_thread;
}

void thread_wake(tid_t tid)
{
    struct thread *prev;
    struct thread *node;
    int find_from_waiting = 0;

    cli();

    if (!thread_find_by_tid(tid, THREAD_READY, &prev, &node) ||
        (find_from_waiting = 1, !thread_find_by_tid(tid, THREAD_WAITING, &prev, &node)))
    {
        if (prev != NULL)
        {
            prev->next = node->next;
        }
        else
        {
            if (find_from_waiting)
            {
                /* 节点为thread_head[THREAD_WAITING] */
                thread_head[THREAD_WAITING] = node->next;
            }
            else
            {
                /* 节点为thread_head[THREAD_READY] */
                thread_head[THREAD_READY] = node->next;
            }
        }
        node->status = THREAD_RUNNING;
        node->next = thread_head[THREAD_RUNNING];
        thread_head[THREAD_RUNNING] = node;

        if (node->context.esp == 0)
        {
            uint32_t *stack_top;
            stack_top = (uint32_t *)((uint32_t)node + KERNEL_THREAD_STACK_SIZE - sizeof(uint32_t) * 3);
            stack_top[0] = (uint32_t)node->handler;
            stack_top[1] = (uint32_t)thread_died;
            stack_top[2] = (uint32_t)node->params;

            node->context.esp = (uint32_t)stack_top;
        }
    }

    sti();
}

void thread_suspend(tid_t tid)
{
    struct thread *prev;
    struct thread *node;

    /* thread cleaner不能暂停 */
    if (tid == 0)
    {
        return;
    }

    cli();

    if (!thread_find_by_tid(tid, THREAD_RUNNING, &prev, &node))
    {
        struct thread *current = thread_current();

        if (prev != NULL)
        {
            prev->next = node->next;
        }
        else
        {
            /* node节点为thread_head[THREAD_RUNNING] */
            thread_head[THREAD_RUNNING] = node->next;
        }

        /* 如果是当前线程暂停自己就要进行切换修复 */
        if (tid == current->tid && (fix_next_thread = node->next) == NULL)
        {
            fix_next_thread = thread_head[THREAD_RUNNING];
        }

        node->status = THREAD_WAITING;
        node->next = thread_head[THREAD_WAITING];
        thread_head[THREAD_WAITING] = node;

        if (tid == current->tid)
        {
            /* 立即切出线程，中断将会在此重新打开 */
            thread_schedule();
        }
    }

    sti();
}

void thread_wait(tid_t tid)
{
    struct thread *node;
    struct thread *prev;

    /* thread cleaner不能被等待 */
    if (tid == 0)
    {
        return;
    }

    cli();

    /* 不在THREAD_DIED中搜索，进入THREAD_DIED后ref值不再改变，以防止死锁发生 */
    if (!thread_find_by_tid(tid, THREAD_RUNNING, &prev, &node) ||
        !thread_find_by_tid(tid, THREAD_WAITING, &prev, &node))
    {
        ++(node->ref);

        sti();

        while (node->status != THREAD_DIED) {}

        --(node->ref);
    }

    sti();
}

void thread_exit(tid_t tid)
{
    struct thread *node;
    struct thread *prev;

    /* thread cleaner不能结束 */
    if (tid == 0)
    {
        return;
    }

    cli();

    if (!thread_find_by_tid(tid, THREAD_RUNNING, &prev, &node))
    {
        struct thread *current = thread_current();

        if (prev != NULL)
        {
            prev->next = node->next;
        }
        else
        {
            /* node节点为thread_head[THREAD_RUNNING] */
            thread_head[THREAD_RUNNING] = node->next;
        }

        /* 如果是当前线程暂停自己就要进行切换修复 */
        if (tid == current->tid && (fix_next_thread = node->next) == NULL)
        {
            fix_next_thread = thread_head[THREAD_RUNNING];
        }

        node->status = THREAD_DIED;
        node->next = thread_head[THREAD_DIED];
        thread_head[THREAD_DIED] = node;

        if (tid == current->tid)
        {
            /* 立即切出线程，中断将会在此重新打开 */
            thread_schedule();
        }
    }

    sti();
}

struct thread *thread_list(enum THREAD_STATUS status)
{
    ASSERT(status < THREAD_STATUS_SIZE);

    return thread_head[status];
}

void print_thread()
{
    int len;
    struct thread *node;
    enum THREAD_STATUS status = 0;
    const char *status_title[] =
    {
        [THREAD_RUNNING] = "running",
        [THREAD_READY] = "ready",
        [THREAD_WAITING] = "waiting",
        [THREAD_DIED] = "died"
    };

    cli();

    set_color_invert();
    printk(" thread name     | tid       | status  ");
    set_color_invert();
    printk("\n");
    while (status < THREAD_STATUS_SIZE)
    {
        node = thread_head[status];
        while (node != NULL)
        {
            len = printk(" %s", node->name);
            while (len < KERNEL_THREAD_NAME_LEN)
            {
                printk(" ");
                ++len;
            }
            printk(" | ");
            len = printk("%d", node->tid);
            while (len < 10)
            {
                printk(" ");
                ++len;
            }
            printk("| %s\n", status_title[status]);
            node = node->next;
        }
        ++status;
    }

    node = thread_head[status];

    sti();
}
