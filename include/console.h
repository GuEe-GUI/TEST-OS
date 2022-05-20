#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <types.h>

#define CONSOLE_FILL    0x44883cff
#define CONSOLE_CLEAR   0x1e2229ff

#define CONSOLE_BACK_FORCE  '\27'

void init_console(uint32_t width, uint32_t height);

void console_dump_version(void);

uint32_t get_console_cols(void);
uint32_t get_console_rows(void);

void console_out(const char *string, uint32_t color, uint32_t background);
void console_roll(void);
void console_cls(void);

#endif /* _CONSOLE_H_ */
