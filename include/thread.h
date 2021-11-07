#ifndef _THREAD_H_
#define _THREAD_H_

#include <types.h>

#define KERNEL_THREAD_STEP          2
#define KERNEL_THREAD_MAX           (1 << 8)   /* 256 */
#define KERNEL_THREAD_STACK_SIZE    (4 * KB)

enum THREAD_STATUS
{
    THREAD_RUNNING = 0,
    THREAD_READY,
    THREAD_WAITING,
    THREAD_DIED,
    THREAD_STATUS_SIZE
};

struct thread_context
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t esp;
};

typedef uint32_t tid_t;

struct thread
{
    tid_t tid;
    enum THREAD_STATUS status;

    void (*handler)(void *params);
    void *params;
    void *ret;

    struct thread_context context;

    struct thread *next;
};

void __attribute__((noreturn)) start_thread(void *handler);
void thread_schedule();
int thread_create(tid_t *tid, void *handler, void *params);
void thread_setup(tid_t tid);
void thread_exit(tid_t tid);

#endif
