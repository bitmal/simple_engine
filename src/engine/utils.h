#ifndef __UTILS_H
#define __UTILS_H

#include "str_view.h"
#include "types.h"

#include <float.h>
#include <stdio.h>

#define UTILS_SORT_RESULT_BEFORE -1
#define UTILS_SORT_RESULT_EQUALS 0
#define UTILS_SORT_RESULT_AFTER 1

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

i32
utils_fprintf(FILE *filePtr, const char *formatStrPtr, ...);

i32
utils_printf(const char *formatStrPtr, ...);

b32 
utils_generate_hash_from_string(const char *k, struct utils_string_hash *outResult);

b32 
utils_generate_string_from_hash(struct utils_string_hash *hashPtr, char *outStr);

void
utils_set_elapsed_time_ns_ptr(u64 *ptr);

void
utils_set_elapsed_time_ms_ptr(u32 *ptr);

u64
utils_get_elapsed_ns();

u32
utils_get_elapsed_ms();

b32
utils_set_random_seed(u32 seed);

b32
utils_get_random_seed(u32 *outSeedPtr);

b32
utils_get_is_random_seed_set();

b32
utils_generate_random_u64(u64 *outRandomPtr);

b32
utils_generate_random_positive_normalized_real64(real64 *outRandomPtr);

b32
utils_generate_random_u64_from_string(const char *str, u64 *outRandomPtr);

real64
utils_generate_random_positive_normalized_real64_from_string(const char *str, real64 *outRandomPtr);

b32
utils_generate_random_sign64(i64 *outSignPtr);

i32
utils_sort_compare64(void *database, void *lhs, void *rhs, u32 elementSize, void *userPtr);

u64
utils_generate_next_prime_number(u64 startingValue);

// use with caution...
#define UTILS_MUTABLE_CAST(type, var) (*(type *)&(var))

#endif