#ifndef __UTILS_H
#define __UTILS_H

#include "str_view.h"
#include "types.h"

#include <float.h>

#define UTILS_SORT_RESULT_BEFORE -1
#define UTILS_SORT_RESULT_EQUALS 0
#define UTILS_SORT_RESULT_AFTER 1

struct memory;

// before(-1) < ==(0) < after(1) 
typedef i32 (*utils_sort_compare_func)(void *database, void *lhs, void *rhs, u32 elementSize, void *userPtr);
typedef void (*utils_sort_replace_func)(void *database, void *lhsPtr, void *rhsPtr, u32 elementSize, 
    void *userPtr);

struct utils_string_hash
{
    u64 stringByteSize;
    u64 hash;
};

typedef u64 utils_hash_u64;
typedef utils_hash_u64 utils_hash;

b32 
utils_generate_hash_from_string(const char *k, struct utils_string_hash *outResult);

b32 
utils_generate_string_from_hash(utils_hash hash, u64 outStrByteSize, char *outStr);

void
utils_set_elapsed_time_ptr(u64 *ptr);

void
utils_set_elapsed_time_int_ptr(u32 *ptr);

u64
utils_get_elapsed_ns();

u32
utils_get_elapsed_ms();

// seedPtr intended length: 3 elements (48 bytes)
u64
utils_generate_random_u64(u16 *seedPtr);

real64
utils_generate_random_positive_normalized_real64(i64 *seedPtr);

#define UTILS_GENERATE_RANDOM_POSITIVE_REAL64(seedPtr) (utils_generate_random_positive_normalized_real64(seedPtr)*DBL_MAX)

u64
utils_generate_random_u64_from_string(const char *str);

real64
utils_generate_random_positive_normalized_real64_from_string(const char *str);

i64
utils_generate_random_sign64(u16 *seedPtr);

i32
utils_sort_compare64(void *database, void *lhs, void *rhs, u32 elementSize, void *userPtr);

// use with caution...
#define UTILS_MUTABLE_CAST(type, var) (*(type *)&(var))

#endif