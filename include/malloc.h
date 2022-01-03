#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <bitmap.h>

#define malloc(size)        bitmap_malloc(size)
#define free(addr)          bitmap_free(addr)
#define realloc(addr, size) bitmap_realloc(addr, size);

#endif /* _MALLOC_H_ */
