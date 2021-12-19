#ifndef _TYPES_H_
#define _TYPES_H_

#ifndef NULL
#define NULL ((void *)0)
#endif /* NULL */

#ifndef __cplusplus
#define bool    _Bool
#define true    1
#define false   0
#else
#define _Bool   bool
#define bool    bool
#define false   false
#define true    true
#endif /* __cplusplus */

typedef unsigned long long  uint64_t;
typedef unsigned int        uint32_t;
typedef unsigned short      uint16_t;
typedef unsigned char       uint8_t;

typedef long long           int64_t;
typedef signed int          int32_t;
typedef signed short        int16_t;
typedef signed char         int8_t;

typedef unsigned long       uintptr_t;
typedef long                intptr_t;

typedef unsigned long       size_t;
typedef unsigned long       off_t;

#define KB 1024
#define MB (1024 * KB)
#define GB (1024 * MB)

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define PTR_LIST(ptr, ...) (void *[]){ptr, ##__VA_ARGS__}
#define PTR_LIST_ITEM(list, n) ((void **)list)[n]

#define IS_POWER_OF_2(val) (val > 0 && (val & (val - 1)) == 0)

#define INTEGER_SWAP(a, b) \
{ \
    a ^= b; \
    b ^= a; \
    a ^= b; \
}

#define TO_LOWER(ch) (ch | ' ')
#define TO_UPPER(ch) (ch & '_')
#define TO_CAPS(ch)  (ch ^ ' ')

#endif /* _TYPES_H_ */
