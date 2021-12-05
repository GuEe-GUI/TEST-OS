#ifndef _EVAl_H_
#define _EVAl_H_

struct eval_void
{
    uint32_t magic;
    const char *name;
    const char *info;
    void (*handler)(int argc, char**argv);
} __attribute__((packed));

#define EVAL_VOID_MAGIC 0xfaceface

#define EVAL_VOID(handler, info) \
void handler(int argc, char**argv);\
const struct eval_void eval_void_##handler __attribute__((used,section(".eval_void"))) = \
{ \
    EVAL_VOID_MAGIC, \
    #handler, \
    info, \
handler}; \
void handler

#define EVAL_CHAR_MAX   1024
#define EVAL_ARGS_MAX   32

#define EVAL_COLOR_INFO     0x1583d7ff
#define EVAL_COLOR_WARN     0xffc107ff
#define EVAL_COLOR_ERROR    0xf44336ff

void eval();

#endif
