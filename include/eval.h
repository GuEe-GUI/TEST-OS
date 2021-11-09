#ifndef _EVAl_H_
#define _EVAl_H_

struct eval_void
{
    uint32_t magic;
    const char *name;
    const char *info;
    void (*handler)();
} __attribute__((packed));

#define EVAL_VIOD_MAGIC 0xfaceface

#define EVAL_VIOD(handler, info) \
void handler(int argc, char**argv);\
const struct eval_void eval_void_##handler __attribute__((used,section(".eval_void"))) = \
{ \
    EVAL_VIOD_MAGIC, \
    #handler, \
    info, \
handler}; \
void handler

void eval();

#endif
