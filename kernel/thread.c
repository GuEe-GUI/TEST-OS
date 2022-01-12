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
static tid_t tid_bitmap[KERNEL_THREAD_MAX / TID_BIT_NR] = {1};  /* 0号tid为缺省值 */

pe_lock_t thread_lock = 0;

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
    struct thread *thread = current_thread;

    thread_exit(thread->tid);

    /* 当前线程执行thread_exit后就会调度出去，并在销毁队列等待销毁，因此如果还能运行在此说明调度器一定有问题 */
    PANIC("thread scheduler out of control");
}

void __attribute__((noreturn)) start_thread(void *thread_list)
{
    /* thread_cleaner 使用原来内核的栈顶 */
    struct thread *thread_cleaner = (struct thread *)(KERNEL_STACK_TOP - KERNEL_THREAD_STACK_SIZE);

    if (ARRAY_SIZE(tid_bitmap) > 1)
    {
        memset((void *)&tid_bitmap[1], 0, sizeof(tid_bitmap) - sizeof(tid_bitmap[0]));
    }

    if ((thread_head[THREAD_READY] = thread_cleaner) == NULL)
    {
        PANIC("init thread cleaner fail");
    }

    thread_cleaner->tid = 0;
    thread_cleaner->status = THREAD_READY;
    thread_cleaner->wake_millisecond = 0;
    thread_cleaner->ref = 0;
    strcpy(thread_cleaner->name, "thread cleaner");
    thread_cleaner->handler = &&thread_cleaner_entry;
    thread_cleaner->params = NULL;
    thread_cleaner->context.esp = KERNEL_STACK_TOP - sizeof(uint32_t) * 3;

    thread_wake(thread_cleaner->tid);
    LOG("%s handler = 0x%p, tid = %d", thread_cleaner->name, thread_cleaner->handler, thread_cleaner->tid);

    if (thread_list != NULL)
    {
        tid_t tid;
        int i = 0;
        while (PTR_LIST_ITEM(thread_list, i) != NULL)
        {
            char *name = (char *)PTR_LIST_ITEM(PTR_LIST_ITEM(thread_list, i), 0);
            void *handler = (char *)PTR_LIST_ITEM(PTR_LIST_ITEM(thread_list, i), 1);
            void *params = (char *)PTR_LIST_ITEM(PTR_LIST_ITEM(thread_list, i), 2);

            ++i;

            if (thread_create(&tid, name, handler, params))
            {
                PANIC("create %s thread fail", name);
            }

            thread_wake(tid);
            LOG("%s thread handler = 0x%p, tid = %d", name, handler, tid);
        }
    }

    cli();
    /* 允许线程调度，第一个执行的线程为thread_cleaner */
    current_thread = thread_cleaner;

    /* 主动恢复thread_cleaner的esp */
    __asm__ volatile ("movl %0, %%esp"::"r"(thread_cleaner->context.esp));

thread_cleaner_entry:
    sti();

    struct thread *prev, *next, *node;

    for (;;)
    {
        prev = NULL;
        node = thread_head[THREAD_DIED];

        while (node != NULL)
        {
            if (node->ref == 0)
            {
                cli();

                next = node->next;

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

                sti();

                free(node);

                node = next;

                continue;
            }
            prev = node;
            node = node->next;
        }
        /* 可销毁节点已空，让出cpu使用权 */
        thread_yield();
    }
}

void thread_schedule(void)
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
        pe_lock(&thread_lock);

        memset(new_thread, 0, KERNEL_THREAD_STACK_SIZE);

        for (map_idx = 0; map_idx < sizeof(tid_bitmap) / sizeof(tid_bitmap[0]); ++map_idx)
        {
            for (bit_idx = 0; bit_idx < (sizeof(tid_t) * 8); ++bit_idx)
            {
                if (((tid_bitmap[map_idx] >> bit_idx) & 1) == 0)
                {
                    uint32_t *stack_top;

                    tid_bitmap[map_idx] |= 1 << bit_idx;

                    *tid = map_idx * (sizeof(tid_t) * 8) + bit_idx;
                    new_thread->tid = *tid;
                    new_thread->status = THREAD_READY;
                    new_thread->wake_millisecond = 0;
                    new_thread->ref = 0;
                    new_thread->handler = handler;
                    new_thread->params = params;

                    /* 根据栈结构填充对应数据 */
                    stack_top = (uint32_t *)((uint32_t)new_thread + KERNEL_THREAD_STACK_SIZE - sizeof(uint32_t) * 3);
                    stack_top[0] = (uint32_t)new_thread->handler;   /* 函数入口地址 */
                    stack_top[1] = (uint32_t)thread_died;           /* 函数返回地址 */
                    stack_top[2] = (uint32_t)new_thread->params;    /* 函数参数值 */

                    new_thread->context.esp = (uint32_t)stack_top;

                    if (name == NULL)
                    {
                        name = "unnamed";
                    }
                    memset(new_thread->name, 0, KERNEL_THREAD_NAME_LEN);
                    strncpy(new_thread->name, name, KERNEL_THREAD_NAME_LEN);
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
        pe_unlock(&thread_lock);
    }

    return status;
}

struct thread *thread_current(void)
{
    ASSERT(current_thread != NULL);

    return current_thread;
}

void thread_wake(tid_t tid)
{
    struct thread *prev, *node;
    int find_from_suspend = 0;

    pe_lock(&thread_lock);

    if (!thread_find_by_tid(tid, THREAD_READY, &prev, &node) ||
        (find_from_suspend = 1, !thread_find_by_tid(tid, THREAD_SUSPEND, &prev, &node)))
    {
        if (prev != NULL)
        {
            prev->next = node->next;
        }
        else
        {
            if (find_from_suspend)
            {
                /* 节点为thread_head[THREAD_SUSPEND] */
                thread_head[THREAD_SUSPEND] = node->next;
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
    }

    pe_unlock(&thread_lock);
}

void thread_suspend(tid_t tid)
{
    struct thread *prev, *node, *current = thread_current();

    /* thread cleaner不能暂停 */
    if (tid == 0)
    {
        return;
    }

    pe_lock(&thread_lock);

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

        /* 如果是当前线程暂停自己就要进行切换修复 */
        if (tid == current->tid && (fix_next_thread = node->next) == NULL)
        {
            fix_next_thread = thread_head[THREAD_RUNNING];
        }

        node->status = THREAD_SUSPEND;
        node->next = thread_head[THREAD_SUSPEND];
        thread_head[THREAD_SUSPEND] = node;

        if (tid == current->tid)
        {
            pe_unlock(&thread_lock);

            /* 立即切出线程，中断将会在此重新打开 */
            thread_yield();
        }
    }

    pe_unlock(&thread_lock);
}

void thread_wait(tid_t tid)
{
    struct thread *node, *prev;

    /* thread cleaner不能被等待 */
    if (tid == 0)
    {
        return;
    }

    pe_lock(&thread_lock);

    /* 不在THREAD_DIED中搜索，进入THREAD_DIED后ref值不再改变，以防止死锁发生 */
    if (!thread_find_by_tid(tid, THREAD_RUNNING, &prev, &node) ||
        !thread_find_by_tid(tid, THREAD_SUSPEND, &prev, &node))
    {
        ++(node->ref);

        pe_unlock(&thread_lock);
        sti();

        while (node->status != THREAD_DIED)
        {
            thread_yield();
        }

        --(node->ref);
    }

    pe_unlock(&thread_lock);
}

void thread_exit(tid_t tid)
{
    struct thread *node, *prev, *current = thread_current();
    int find_from_suspend = 0;

    /* thread cleaner不能结束 */
    if (tid == 0)
    {
        return;
    }

    pe_lock(&thread_lock);

    if (!thread_find_by_tid(tid, THREAD_RUNNING, &prev, &node) ||
        (find_from_suspend = 1, !thread_find_by_tid(tid, THREAD_SUSPEND, &prev, &node)))
    {

        if (prev != NULL)
        {
            prev->next = node->next;
        }
        else
        {
            if (find_from_suspend)
            {
                /* 节点为thread_head[THREAD_SUSPEND] */
                thread_head[THREAD_SUSPEND] = node->next;
            }
            else
            {
                /* node节点为thread_head[THREAD_RUNNING] */
                thread_head[THREAD_RUNNING] = node->next;
            }
        }

        /* 如果是当前线程暂停自己就要进行切换修复 */
        if (tid == current->tid && (fix_next_thread = node->next) == NULL)
        {
            fix_next_thread = thread_head[THREAD_RUNNING];
        }

        /* 确认线程是否使用sleep睡眠 */
        if (node->status == THREAD_SUSPEND && node->wake_millisecond != 0 && node->ref > 0)
        {
            /* 使用sleep的线程ref大于0，要减一才可能被清理 */
            --(node->ref);
        }

        node->status = THREAD_DIED;
        node->next = thread_head[THREAD_DIED];
        thread_head[THREAD_DIED] = node;

        if (tid == current->tid)
        {
            pe_unlock(&thread_lock);

            /* 立即切出线程，中断将会在此重新打开 */
            thread_yield();
        }
    }

    pe_unlock(&thread_lock);
}

struct thread *thread_list(enum THREAD_STATUS status)
{
    ASSERT(status < THREAD_STATUS_SIZE);

    return thread_head[status];
}

void print_thread(void)
{
    int len;
    struct thread *node;
    enum THREAD_STATUS status = 0;
    const char *status_title[] =
    {
        [THREAD_RUNNING] = "running",
        [THREAD_READY] = "ready",
        [THREAD_SUSPEND] = "suspend",
        [THREAD_DIED] = "died"
    };

    pe_lock(&thread_lock);

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
            put_space(len, KERNEL_THREAD_NAME_LEN);
            printk(" | ");
            len = printk("%d", node->tid);
            put_space(len, 10);
            printk("| %s\n", status_title[status]);
            node = node->next;
        }
        ++status;
    }

    pe_unlock(&thread_lock);
}
