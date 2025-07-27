#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>

static const u64 *g_ELAPSED_TIME_PTR;
static const u32 *g_ELAPSED_TIME_INT_PTR;
static u32 g_RANDOM_SEED;
static b32 g_IS_RANDOM_SEED_SET;

i32
utils_fprintf(FILE *filePtr, const char *formatStrPtr, ...)
{
    va_list args;
    va_start(args, formatStrPtr);

    i32 resultCode = vfprintf(filePtr, formatStrPtr, args);

    va_end(args);

    return resultCode;
}

i32
utils_printf(const char *formatStrPtr, ...)
{
    va_list args;
    va_start(args, formatStrPtr);

    i32 resultCode = vprintf(formatStrPtr, args);

    va_end(args);

    return resultCode;
}

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
utils_generate_string_from_hash(struct utils_string_hash *hashPtr, char *outStr)
{
    if (outStr)
    {
        const i32 p = 59;
        const i32 m = 1e9 + 9;
        u64 hash = hashPtr->hash;
        u64 pPow = (pPow*p)*(hashPtr->stringByteSize - 1)%m;

        u64 lastCharNumber = hashPtr->stringByteSize;

        for (u64 charIndex = lastCharNumber - 1; charIndex >= 0; --charIndex)
        {
            //outStr[charIndex] = ;
            hash = (hash - (outStr[charIndex] - 'a' + 1)*pPow%m)%m;
            pPow -= (pPow*p)%m;
        }

        outStr[lastCharNumber] = '\0';

        return B32_FALSE;
    }

    return B32_FALSE;
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
utils_set_random_seed(u32 seed)
{
    // TODO: see initstate and setstate functions in cstdlib for controlling the numbers generated,
    // the quality of random using the state, and the amount of numbers used in random generation.
    // this is useful for generating the same 'random' values each time (or different), using the same seed
    // across running sessions
    if ((!g_IS_RANDOM_SEED_SET) ||
        (seed != g_RANDOM_SEED))
    {
        g_RANDOM_SEED = seed;

        srand(seed);

        g_IS_RANDOM_SEED_SET = B32_TRUE;

        return B32_TRUE;
    }

    return B32_FALSE;
}

b32
utils_get_random_seed(u32 *outSeedPtr)
{
    if (g_IS_RANDOM_SEED_SET && outSeedPtr)
    {
        *outSeedPtr = g_RANDOM_SEED;

        return B32_TRUE;
    }

    return B32_FALSE;
}

b32
utils_get_is_random_seed_set()
{
    return g_IS_RANDOM_SEED_SET;
}

b32
utils_generate_random_u64(u64 *outRandomPtr)
{
    if (g_IS_RANDOM_SEED_SET && outRandomPtr)
    {
        *outRandomPtr = ((real64)rand()/RAND_MAX)*UINT64_MAX;

        return B32_TRUE;
    }

    return B32_FALSE;
}

b32
utils_generate_random_positive_normalized_real64(real64 *outRandomPtr)
{
    if (g_IS_RANDOM_SEED_SET && outRandomPtr)
    {
        *outRandomPtr = (real64)rand()/RAND_MAX;
        
        return B32_TRUE;
    }

    return B32_FALSE;
}

b32
utils_generate_random_u64_from_string(const char *str, u64 *outRandomPtr)
{
    if (g_IS_RANDOM_SEED_SET && outRandomPtr)
    {
        struct utils_string_hash resultHash;

        utils_generate_hash_from_string(str, &resultHash);

        *outRandomPtr = (u64)((real64)rand()/RAND_MAX*resultHash.hash);

        return B32_TRUE;
    }
    
    return B32_FALSE;
}

real64
utils_generate_random_positive_normalized_real64_from_string(const char *str, real64 *outRandomPtr)
{
    if (g_IS_RANDOM_SEED_SET && outRandomPtr)
    {
        struct utils_string_hash resultHash;
        
        utils_generate_hash_from_string(str, &resultHash);

        *outRandomPtr = (real64)rand()/RAND_MAX*(real64)resultHash.hash;

        return B32_TRUE;
    }
    
    return B32_FALSE;
}

b32
utils_generate_random_sign64(i64 *outSignPtr)
{
    if (g_IS_RANDOM_SEED_SET && outSignPtr)
    {
        return (i64)(round((real64)rand()/RAND_MAX));

        return B32_TRUE;
    }

    return B32_FALSE;
}

i32
utils_sort_compare64(void *database, void *lhs, void *rhs, u32 elementSize, void *userPtr)
{
    assert(B32_FALSE);

    // TODO:

    return 0;
}

u64
utils_generate_next_prime_number(u64 startingValue)
{
    u64 result;

    for (result = startingValue;;)    
    {
        if ((result%2) > 0)
        {
            b32 isPrime = B32_TRUE;
            u64 halfResult = result/2;

            for (u64 i = 2; i <= halfResult;)
            {
                if ((result%i) == 0)
                {
                    isPrime = B32_FALSE;
                    
                    break;
                }

                if ((i%2) > 0)
                {
                    i += 2;
                }
                else 
                {
                    ++i;
                }
            }
                
            if (isPrime)
            {
                break;
            }

            result += 2;
        }
        else 
        {
            ++result;
        }
    }

    return result;
}