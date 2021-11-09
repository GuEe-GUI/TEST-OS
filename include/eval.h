#ifndef _EVAl_H_
#define _EVAl_H_

struct eval_void
{
    const char *name;
    const char *info;
    void (*handler)();
} __attribute__((packed));

#define EVAL_VIOD(handler, info) \
void handler(int argc, char**argv);\
const struct eval_void eval_void_##handler __attribute__((used,section(".eval_void_start"))) = {#handler, info, handler}; \
void handler\

void eval();

#endif
