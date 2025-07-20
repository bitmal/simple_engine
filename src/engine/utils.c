#include "utils.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

static const u64 *g_ELAPSED_TIME_PTR;
static const u32 *g_ELAPSED_TIME_INT_PTR;
static u16 g_RANDOM_SEED_U64[3];
b32 g_IS_RANDOM_SEED_U64_SET;
static i64 g_RANDOM_SEED_REAL64;
b32 g_IS_RANDOM_SEED_REAL64_SET;

b32 
utils_generate_hash_from_string(const char *k, struct utils_string_hash *outResult)
{
    if (!outResult || !k)
    {
        return B32_FALSE;
    }

    // https://cp-algorithms.com/string/string-hashing.html
    // # 'Polynomial Rolling Hash Function'
    // # scroll to 'Calculation of the Hash of a String'

    const i32 p = 59;
    const i32 m = 1e9 + 9;
    u64 hash = 0;
    u64 pPow = 1;

    for (const char *c = k; *c != '\0'; ++c)
    {
        hash = (hash + (*c - 'a' + 1)*pPow)%m;
        pPow = (pPow*p)%m;
    }

    outResult->hash = hash;
    outResult->stringByteSize = strlen(k) + 1;

    return B32_TRUE;
}

b32 
utils_generate_string_from_hash(utils_hash hash, u64 outStrByteSize, char *outStr)
{
    // TODO: reverse the hash into a string
    assert(B32_FALSE);

    if (!outStr)
    {
        return B32_FALSE;
    }

    return B32_TRUE;
}

void
utils_set_elapsed_time_ns_ptr(u64 *ptr)
{
    g_ELAPSED_TIME_PTR = ptr;
}

void
utils_set_elapsed_time_ms_ptr(u32 *ptr)
{
    g_ELAPSED_TIME_INT_PTR = ptr;
}

u64
utils_get_elapsed_ns()
{
    return g_ELAPSED_TIME_PTR ? (*g_ELAPSED_TIME_PTR) : 0;
}

u32
utils_get_elapsed_ms()
{
    return g_ELAPSED_TIME_INT_PTR ? (*g_ELAPSED_TIME_INT_PTR) : 0;
}

b32
utils_set_random_seed_u64(u16 seedPtr[3])
{
    assert(seedPtr);

    if (!seedPtr)
    {
        return B32_FALSE;
    }

    if (!g_IS_RANDOM_SEED_U64_SET ||
        ((((u32 *)g_RANDOM_SEED_U64)[0] != ((u32 *)seedPtr)[0]) ||
        g_RANDOM_SEED_U64[2] != seedPtr[2])) 
    {
        ((u32 *)g_RANDOM_SEED_U64)[0] = ((u32 *)seedPtr)[0];
        g_RANDOM_SEED_U64[2] = seedPtr[2];
        
        seed48(g_RANDOM_SEED_U64);

        g_IS_RANDOM_SEED_U64_SET = B32_TRUE;

        return B32_TRUE;
    }

    return B32_FALSE;
}

b32
utils_set_random_seed_real64(i64 seed)
{
    if (!g_IS_RANDOM_SEED_REAL64_SET || (g_RANDOM_SEED_REAL64 != seed))
    {
        g_RANDOM_SEED_REAL64 = seed;
        
        srand48(g_RANDOM_SEED_REAL64);
        
        g_IS_RANDOM_SEED_REAL64_SET = B32_TRUE;
        
        return B32_TRUE;
    }

    return B32_FALSE;
}

b32
utils_get_is_random_seed_u64_set()
{
    return g_IS_RANDOM_SEED_U64_SET;
}

b32
utils_get_is_random_seed_real64_set()
{
    return g_IS_RANDOM_SEED_REAL64_SET;
}

u64
utils_generate_random_u64()
{
    if (g_IS_RANDOM_SEED_REAL64_SET)
    {
        return (u64)lrand48();
    }

    return 0;
}

real64
utils_generate_random_positive_normalized_real64()
{
    if (g_RANDOM_SEED_REAL64)
    {
        return drand48();
    }

    return 0.;
}

i64
utils_generate_random_sign64()
{
    if (g_IS_RANDOM_SEED_U64_SET)
    {
        return (i64)(round((real64)lrand48()/INT64_MAX));
    }

    return 0;
}

i32
utils_sort_compare64(void *database, void *lhs, void *rhs, u32 elementSize, void *userPtr)
{
    assert(B32_FALSE);

    // TODO:

    return 0;
}