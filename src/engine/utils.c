#include "utils.h"
#include "memory.h"

#include <assert.h>

static const u32 *g_ELAPSED_TIME_PTR = NULL;

u64 
utils_str_hash_func(const char *k)
{
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
    
    return hash;
}

void
utils_sort(struct memory *mem, void *data, size_t stride, size_t length, utils_sort_compare_func compareFunc, 
           utils_sort_replace_func replaceFunc, void *userPtr)
{
    void *elementCopy = memory_alloc(mem, stride);

    u8 *begin = data;
    const u8 *end = (u8 *)data + length*stride;

    for (u8 *iter = begin + stride; iter != end; iter += stride)
    {
        u8 *location;
        u8 *element = iter;
        memcpy(elementCopy, element, stride);
        const size_t elementIndex = (iter - begin)/stride;

        size_t minIndex = 0;
        u8 *minElement = begin;
        size_t range = length;

        while(B32_TRUE)
        {
            const size_t halfIndex = minIndex + (range/2 - 1);
            const size_t maxIndex = minIndex + (range - 1);
            void *halfElement = begin + (halfIndex*stride);
            void *maxElement = begin + (maxIndex*stride);

            if (compareFunc(mem, minElement, minIndex, element, elementIndex, userPtr) >= 0)
            {
                location = minElement;
                break;
            }
            else if (compareFunc(mem, halfElement, halfIndex, element, elementIndex, userPtr) == 0)
            {
                location = halfElement;
                break;
            }
            else if (compareFunc(mem, maxElement, maxIndex, element, elementIndex, userPtr) >= 0)
            {
                location = maxElement;
                break;
            }
            else
            {
                if (compareFunc(mem, halfElement, halfIndex, element, elementIndex, userPtr) < 0)
                {
                    range = halfIndex - minIndex;

                    minElement += stride;

                    ++minIndex;
                }
                else
                {
                    range = maxIndex - halfIndex;
                    minElement = (u8 *)halfElement + stride;
                }
            }
        }

        u8 *j = (u8 *)element - stride;
        size_t jdex = elementIndex - 1;

        for (; j >= location; j -= stride, --jdex)
        {
            replaceFunc(mem, j + stride, jdex + 1, j, jdex, stride, userPtr);
        }

        replaceFunc(mem, j + stride, jdex + 1, elementCopy, elementIndex, stride, userPtr);
    }

    memory_free(mem, elementCopy);
}

void
utils_set_elapsed_time_ptr(const u32 *ptr)
{
    g_ELAPSED_TIME_PTR = ptr;
}

b32
utils_get_elapsed_ms(u32 *outputPtr)
{
    assert(outputPtr);

    if (g_ELAPSED_TIME_PTR)
    {
        *outputPtr = *g_ELAPSED_TIME_PTR;

        return B32_TRUE;
    }

    return B32_FALSE;
}