#include <io.h>
#include <kernel.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <thread.h>

static struct thread *thread_head[THREAD_STATUS_SIZE] = {NULL};
static struct thread *current_thread = NULL;
static tid_t tid_bitmap[KERNEL_THREAD_MAX / (sizeof(tid_t) * 8)] = {1}; /* 0号tid为缺省值 */

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

void __attribute__((noreturn)) start_thread(void *handler)
{
    tid_t tid;

    thread_head[THREAD_READY] = (struct thread *)malloc(KERNEL_THREAD_STACK_SIZE);

    if (thread_head[THREAD_READY] == NULL)
    {
        PANIC("init thread cleaner fail\n");
    }

    thread_head[THREAD_READY]->tid = 0;
    thread_head[THREAD_READY]->handler = &&thread_cleaner;
    thread_head[THREAD_READY]->params = NULL;

    thread_wake(thread_head[THREAD_READY]->tid);
    LOG("thread cleaner handler = %p, tid = %d\n", thread_head[THREAD_RUNNING]->handler, thread_head[THREAD_RUNNING]->tid);

    if (thread_create(&tid, handler, NULL))
    {
        PANIC("create first thread fail\n");
    }

    thread_wake(tid);
    LOG("first thread handler = %p, tid = %d\n", thread_head[THREAD_RUNNING]->handler, thread_head[THREAD_RUNNING]->tid);

    cli();
    /* 允许线程调度，第一个执行的线程为thread_cleaner，因此为next */
    current_thread = thread_head[THREAD_RUNNING]->next;

    /* 主动恢复thread_cleaner的esp */
    __asm__ volatile ("movl %0, %%esp"::"r"(current_thread->context.esp));

thread_cleaner:
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
                free(node);

                sti();
            }
            prev = node;
            node = node->next;
        }
    }
}

void thread_schedule()
{
    cli();

    if (current_thread != NULL)
    {
        struct thread *prev = current_thread;

        if (current_thread->next != NULL)
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

int thread_create(tid_t *tid, void *handler, void *params)
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
                    new_thread->ref = 0;
                    new_thread->handler = handler;
                    new_thread->params = params;
                    new_thread->ret = thread_died;
                    new_thread->context.esp = 0;

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

void thread_wake(tid_t tid)
{
    struct thread *prev;
    struct thread *node;

    cli();

    if (!thread_find_by_tid(tid, THREAD_READY, &prev, &node))
    {
        if (prev != NULL)
        {
            prev->next = node->next;
        }
        else
        {
            /* 节点为thread_head[THREAD_READY] */
            thread_head[THREAD_READY] = node->next;
        }
        node->status = THREAD_RUNNING;
        node->next = thread_head[THREAD_RUNNING];
        thread_head[THREAD_RUNNING] = node;

        if (node->context.esp == 0)
        {
            uint32_t *stack_top;
            stack_top = (uint32_t *)((uint32_t)node + KERNEL_THREAD_STACK_SIZE - sizeof(uint32_t) * 3);
            stack_top[0] = (uint32_t)node->handler;
            stack_top[1] = (uint32_t)node->ret;
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
        if (prev != NULL)
        {
            prev->next = node->next;
        }
        else
        {
            /* node节点为thread_head[THREAD_RUNNING] */
            thread_head[THREAD_RUNNING] = node->next;
        }
        node->status = THREAD_WAITING;
        node->next = thread_head[THREAD_WAITING];
        thread_head[THREAD_WAITING] = node;
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
        if (prev != NULL)
        {
            prev->next = node->next;
        }
        else
        {
            /* node节点为thread_head[THREAD_RUNNING] */
            thread_head[THREAD_RUNNING] = node->next;
        }
        node->status = THREAD_DIED;
        node->next = thread_head[THREAD_DIED];
        thread_head[THREAD_DIED] = node;
    }

    sti();
}
