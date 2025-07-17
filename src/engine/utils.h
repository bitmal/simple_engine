#ifndef __UTILS_H
#define __UTILS_H

#include "str_view.h"
#include "types.h"

#define UTILS_SORT_BEFORE
#define UTILS_SORT_EQUALS
#define UTILS_SORT_AFTER

struct memory;

// before(-1) < ==(0) < after(1) 
typedef i32 (*utils_sort_compare_func)(struct memory *mem, void *lhs, size_t lhsIndex, void *rhs, size_t rhsIndex, void *userPtr);
typedef void (*utils_sort_replace_func)(struct memory *mem, void *lhs, size_t lhsIndex, void *rhs, size_t rhsIndex, 
    size_t elementSize, void *userPtr);

u64 
utils_str_hash_func(const char *k);

void
utils_sort(struct memory *mem, void *data, size_t stride, size_t length, utils_sort_compare_func compareFunc, 
           utils_sort_replace_func replaceFunc, void *userPtr);

void
utils_set_elapsed_time_ptr(const u32 *ptr);

b32
utils_get_elapsed_ms(u32 *outputPtr);

// use with caution...
#define UTILS_MUTABLE_CAST(type, var) (*(type *)&(var))

#endif