#ifndef _THREAD_H_
#define _THREAD_H_

#include <io.h>
#include <types.h>
#include <timer.h>

#define KERNEL_THREAD_STEP          2
#define KERNEL_THREAD_MAX           (1 << 8)   /* 256 */
#define KERNEL_THREAD_NAME_LEN      16
#define KERNEL_THREAD_STACK_SIZE    (4 * KB)
#define KERNEL_THREAD_CONSOLE_SIZE  512

enum THREAD_STATUS
{
    THREAD_RUNNING = 0,
    THREAD_READY,
    THREAD_SUSPEND,
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
    uint32_t wake_millisecond;
    uint32_t ref;
    char name[KERNEL_THREAD_NAME_LEN];
    char console_cache[KERNEL_THREAD_CONSOLE_SIZE];

    void (*handler)(void *params);
    void *params;

    struct thread_context context;

    struct thread *next;
};

void __attribute__((noreturn)) start_thread(void *thread_list);
void thread_schedule(void);
int thread_create(tid_t *tid, char *name, void *handler, void *params);
struct thread *thread_current(void);
void thread_wake(tid_t tid);
void thread_suspend(tid_t tid);
void thread_wait(tid_t tid);
void thread_exit(tid_t tid);
struct thread *thread_list(enum THREAD_STATUS status);
void print_thread(void);

static inline void thread_yield(void)
{
    thread_schedule();
}

extern pe_lock_t thread_lock;

#endif /* _THREAD_H_ */
