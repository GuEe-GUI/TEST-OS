#include <io.h>
#include <kernel.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <thread.h>

static struct thread *thread_head[THREAD_STATUS_SIZE] = {NULL};
static struct thread *current_thread = NULL;
static tid_t tid_bitmap[KERNEL_THREAD_MAX / (sizeof(tid_t) * 8)] = {1};

extern void switch_to_next_thread(void *prev, void *next);

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

    thread_setup(thread_head[THREAD_READY]->tid);
    LOG("thread cleaner handler = %p, tid = %d\n", thread_head[THREAD_RUNNING]->handler, thread_head[THREAD_RUNNING]->tid);

    if (thread_create(&tid, handler, NULL))
    {
        PANIC("create first thread fail\n");
    }

    thread_setup(tid);
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
        struct thread *next;
        struct thread *node = thread_head[THREAD_DIED];

        while (node != NULL)
        {
            next = node->next;
            free(node);
            node = next;
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
                    new_thread->handler = handler;
                    new_thread->params = params;
                    new_thread->ret = thread_died;

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

void thread_setup(tid_t tid)
{
    struct thread *prev;
    struct thread *node;

    cli();

    prev = NULL;
    node = thread_head[THREAD_READY];

    while (node != NULL)
    {
        if (node->tid == tid)
        {
            uint32_t *stack_top;

            if (prev != NULL)
            {
                prev->next = node->next;
            }
            else
            {
                /* 节点为 thread_head[THREAD_READY] */
                thread_head[THREAD_READY] = node->next;
            }
            node->status = THREAD_RUNNING;
            node->next = thread_head[THREAD_RUNNING];
            thread_head[THREAD_RUNNING] = node;

            stack_top = (uint32_t *)((uint32_t)node + KERNEL_THREAD_STACK_SIZE - sizeof(uint32_t) * 3);
            stack_top[0] = (uint32_t)node->handler;
            stack_top[1] = (uint32_t)node->ret;
            stack_top[2] = (uint32_t)node->params;

            node->context.esp = (uint32_t)stack_top;

            break;
        }
        else if (node->next != NULL)
        {
            prev = node;
            node = node->next;
        }
        else
        {
            break;
        }
    }

    sti();
}

void thread_exit(tid_t tid)
{
    if (tid != 0)
    {
        struct thread *prev;
        struct thread *node;

        cli();

        prev = NULL;
        node = thread_head[THREAD_RUNNING];

        while (node != NULL)
        {
            if (node->tid == tid)
            {
                if (prev != NULL)
                {
                    prev->next = node->next;
                }
                else
                {
                    /* 节点为 is thread_head[THREAD_RUNNING] */
                    thread_head[THREAD_RUNNING] = node->next;
                }
                node->status = THREAD_DIED;
                node->next = thread_head[THREAD_DIED];
                thread_head[THREAD_DIED] = node;

                break;
            }
            else if (node->next != NULL)
            {
                prev = node;
                node = node->next;
            }
            else
            {
                break;
            }
        }

        sti();
    }
}
