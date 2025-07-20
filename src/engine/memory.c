#include "memory.h"
#include "types.h"
#include "utils.h"

#include <stdint.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#if defined(_WIN32) || defined(__WIN32) || defined(WIN32)
    #include <malloc.h>
#elif defined(linux)
    #include <malloc.h>
    #include <stdlib.h>
#endif

#define MEMORY_USER_REGION_PAGE_IDENTIFIER (0xDEAD)

struct memory_user_region_page_header
{
    u16 identifier;
    enum memory_page_status_t status;
    u64 byteSize;
    u16 allocCount;
};

struct memory_safe_ptr
{
    memory_int_id safePtrId;
    memory_short_id pageId;
    p64 allocByteOffset;
    u64 refCount;
    b32 isActive;
};

struct memory_raw_alloc_key
{
    memory_short_id rawAllocId;
    u16 rawAllocMapBranchIndex;
    struct memory_raw_alloc_key *prev;
    struct memory_raw_alloc_key *next;
    b32 isActive;
};

struct memory_alloc_key
{
    memory_int_id allocId;
    memory_int_id safePtrId;
    struct memory_alloc_key *prev;
    struct memory_alloc_key *next;
    b32 isActive;
};

struct memory_user_region_page_allocator
{
    u16 identifier;
    u8 byteOffset;
    u8 __padding;
    u16 _padding[5];
    u64 byteSize;
};

struct memory_context
{
    memory_id id;
    p64 safePtrRegionByteOffset;
    u64 safePtrRegionByteCapacity;
    p64 userRegionByteOffset;
    u64 userRegionByteCapacity;
    struct memory_allocator *allocActiveList;
    struct memory_allocator *allocFreeList;
    struct memory_safe_ptr *safePtrActiveList;
    struct memory_safe_ptr *safePtrFreeList;
    u64 userRegionPageByteOffsets[MEMORY_MAX_USER_REGION_PAGES];
    u8 *heap;
};

struct memory_debug_context
{
    struct memory_context _;

    char label[MEMORY_MAX_NAME_LENGTH + 1];
    p64 labelRegionByteOffset;
    u64 labelRegionByteCapacity;
    u32 eventQueueReadIndex;
    u32 eventQueueWriteIndex;
    u32 eventQueueCapacity;
};

struct memory_event
{
    memory_id eventId;
    enum memory_event_type type;
    u32 elapsedMS;

    union
    {
        struct
        {
            memory_id userSectionId;
            memory_id allocId;
            u64 allocSize;
            u64 allocByteOffset;
        } as_alloc;
        
        struct
        {
            memory_id userSectionId;
            memory_id allocId;
            u64 allocSize;
            u64 allocByteOffset;
        } as_free;
    };
};

static void **g_MEMORY_RAW_ALLOC_ARR;
static u16 g_MEMORY_RAW_ALLOC_COUNT;
static u16 g_MEMORY_RAW_ALLOC_CAPACITY;
static p64 **g_MEMORY_RAW_ALLOC_MAP;
static u16 g_MEMORY_RAW_ALLOC_MAP_BUCKET_COUNT;
static struct memory_raw_alloc_key *g_MEMORY_RAW_ALLOC_KEY_ACTIVE_LIST;
static struct memory_raw_alloc_key *g_MEMORY_RAW_ALLOC_KEY_FREE_LIST;

static memory_error_code
_memory_generate_unique_byte_id(memory_byte_id *outResult)
{
    assert(outResult);
    
    memory_error_code errorResult;

    if ((utils_get_is_random_seed_real64_set()))
    {
        *outResult = (memory_byte_id)(round(UTILS_GENERATE_RANDOM_POSITIVE_REAL64()*UINT8_MAX));
        errorResult = MEMORY_OK;
    }
    else
    {
        *outResult = MEMORY_BYTE_ID_NULL;
        errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
    }

   return errorResult;
}

static memory_error_code
_memory_generate_unique_short_id(memory_short_id *outResult)
{
    assert(outResult);
    
    memory_error_code errorResult;

    if ((utils_get_is_random_seed_real64_set()))
    {
        *outResult = (memory_short_id)(round(UTILS_GENERATE_RANDOM_POSITIVE_REAL64()*UINT16_MAX));
        errorResult = MEMORY_OK;
    }
    else
    {
        *outResult = MEMORY_SHORT_ID_NULL;
        errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
    }

   return errorResult;
}

static memory_error_code
_memory_generate_unique_int_id(memory_int_id *outResult)
{
    assert(outResult);
    
    memory_error_code errorResult;

    if ((utils_get_is_random_seed_real64_set()))
    {
        *outResult = (memory_int_id)(round(UTILS_GENERATE_RANDOM_POSITIVE_REAL64()*UINT32_MAX));
        errorResult = MEMORY_OK;
    }
    else
    {
        *outResult = MEMORY_INT_ID_NULL;
        errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
    }

   return errorResult;
}

static memory_error_code
_memory_generate_unique_id(memory_id *outResult)
{
    assert(outResult);
    
    memory_error_code errorResult;

    if ((utils_get_is_random_seed_u64_set()))
    {
        *outResult = (memory_id)(utils_generate_random_u64());
        errorResult = MEMORY_OK;
    }
    else
    {
        *outResult = MEMORY_INT_ID_NULL;
        errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
    }

   return errorResult;
}

static memory_error_code
_memory_raw_alloc_ok()
{
    static b32 isInit = B32_FALSE;

    if (!isInit)
    {
        isInit = B32_TRUE;
    }

    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

static memory_error_code
_memory_alloc_context(struct memory_context **outMemoryContext)
{
    if (!outMemoryContext)
    {
        return MEMORY_ERROR_NULL_PARAMETER;
    }

    static struct memory_context *memoryContextArr = NULL;
    static u16 memoryContextCount = 0;
    
    static struct memory_debug_context *memoryDebugContextArr = NULL;
    static u16 memoryDebugContextCount = 0;

    #ifdef MEMORY_DEBUG
    if (memoryDebugContextCount > 0)
    {
        struct memory_debug_context *reallocatedMemoryContextPtr = 
            realloc(memoryDebugContextArr, sizeof(struct memory_debug_context)*(memoryDebugContextCount + 1));

        if (reallocatedMemoryContextPtr)
        {
            memoryDebugContextArr = reallocatedMemoryContextPtr;
        }
        else 
        {
            fprintf(stderr, "_memory_alloc_context(%d): Failure to reallocate debug context array. "
                    "Cannot allocate new debug context.\n", __LINE__);

            return MEMORY_ERROR_FAILED_ALLOCATION;
        }
    }
    else 
    {
        memoryDebugContextArr = malloc(sizeof(struct memory_debug_context));
        
        if (!memoryDebugContextArr) 
        {
            fprintf(stderr, "_memory_alloc_context(%d): Failure to allocate debug context array. "
                    "Cannot allocate new debug context.\n", __LINE__);

            return MEMORY_ERROR_FAILED_ALLOCATION;
        }
    }

    *outMemoryContext = (struct memory_context *)&memoryDebugContextArr[memoryDebugContextCount++];
    #else
    {
        if (memoryContextCount > 0)
        {
            struct memory_context *reallocatedMemoryContextPtr = 
                realloc(memoryContextArr, sizeof(struct memory_context)*(memoryContextCount + 1));

            if (reallocatedMemoryContextPtr)
            {
                memoryContextArr = reallocatedMemoryContextPtr;
            }
            else 
            {
                fprintf(stderr, "_memory_alloc_context(%d): Failure to reallocate context array. "
                        "Cannot allocate new context.\n", __LINE__);

                return MEMORY_ERROR_FAILED_ALLOCATION;
            }
        }
        else 
        {
            memoryContextArr = malloc(sizeof(struct memory_context));
            
            if (!memoryDebugContextArr) 
            {
                fprintf(stderr, "_memory_alloc_context(%d): Failure to allocate context array. "
                        "Cannot allocate new context.\n", __LINE__);

                return MEMORY_ERROR_FAILED_ALLOCATION;
            }
        }

        *outMemoryContext = (struct memory_context *)&memoryContextArr[memoryContextCount++];
    }
    #endif
    
    return MEMORY_OK;
}

memory_error_code
memory_alloc_raw_heap(const struct memory_raw_alloc_key *outRawAllocKeyPtr, u64 byteSize)
{
    {
        b32 isOutNull = (!outRawAllocKeyPtr);
        b32 isByteSizeLessThanOne = (byteSize < 1);
        b32 isRawAllocsUnderMaxCount = (g_MEMORY_RAW_ALLOC_COUNT < MEMORY_MAX_RAW_ALLOCS);
        
        if (isOutNull || isByteSizeLessThanOne || isRawAllocsUnderMaxCount)
        {
            memory_error_code resultCode;

            if (isOutNull)
            {
                fprintf(stderr, "memory_alloc_raw_heap(%d): 'outRawAllocId' parameter cannot be NULL.\n", __LINE__);

                resultCode = MEMORY_ERROR_NULL_PARAMETER;
            }
            
            if (byteSize < 1)
            {
                fprintf(stderr, "memory_alloc_raw_heap(%d): ByteSize < 1. Failure to allocate.\n", __LINE__);
                
                resultCode = MEMORY_ERROR_ZERO_PARAMETER;
            }

            if (!isRawAllocsUnderMaxCount)
            {
                fprintf(stderr, "memory_alloc_raw_heap(%d): Too many raw allocations. Cannot reserve memory.\n", __LINE__);

                resultCode = MEMORY_ERROR_FAILED_ALLOCATION;
            }

            return resultCode;
        }
    }

    // generate an ID
    {
        memory_error_code genMemoryIdResultCode = _memory_generate_unique_short_id((memory_short_id *)&outRawAllocKeyPtr->rawAllocId);

        if (genMemoryIdResultCode != MEMORY_OK)
        {
            return genMemoryIdResultCode;
        }
    }

    // allocate necessary memory
    struct memory_raw_alloc_key *keyPtr;

    if (!g_MEMORY_RAW_ALLOC_KEY_FREE_LIST)
    {
    }
    else 
    {
        struct memory_raw_alloc_key *temp = g_MEMORY_RAW_ALLOC_KEY_FREE_LIST;

        if (g_MEMORY_RAW_ALLOC_KEY_FREE_LIST->next != g_MEMORY_RAW_ALLOC_KEY_FREE_LIST)
        {
            g_MEMORY_RAW_ALLOC_KEY_FREE_LIST->prev->next = g_MEMORY_RAW_ALLOC_KEY_FREE_LIST->next;
            g_MEMORY_RAW_ALLOC_KEY_FREE_LIST->next->prev = g_MEMORY_RAW_ALLOC_KEY_FREE_LIST->prev;
        }
        else 
        {
            g_MEMORY_RAW_ALLOC_KEY_FREE_LIST = NULL;
        }

        keyPtr = temp;
    }

    if (g_MEMORY_RAW_ALLOC_KEY_ACTIVE_LIST)
    {
        g_MEMORY_RAW_ALLOC_KEY_ACTIVE_LIST->prev->next = keyPtr;
        g_MEMORY_RAW_ALLOC_KEY_ACTIVE_LIST->next->prev = keyPtr;
    }
    else 
    {
        keyPtr->prev = keyPtr->next = keyPtr;
    }

    g_MEMORY_RAW_ALLOC_KEY_ACTIVE_LIST = keyPtr;

    // initialize

    return MEMORY_OK;
}

memory_error_code
memory_realloc_raw_heap(const struct memory_raw_alloc_key *rawAllocKeyPtr, u64 byteSize)
{
    if (!outResult)
    {
        fprintf(stderr, "memory_realloc_raw_heap(%d): Out parameter cannot be NULL.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_PARAMETER;
    }
    
    if (byteSize < 1)
    {
        fprintf(stderr, "memory_realloc_raw_heap(%d): ByteSize < 1. Failure to reallocate.\n", __LINE__);
        
        return MEMORY_ERROR_ZERO_PARAMETER;
    }

    *outResult = realloc(*outResult, byteSize);

    return MEMORY_OK;
}

memory_error_code
memory_free_raw_heap(u8 **heap)
{
    if (!heap)
    {
        fprintf(stderr, "memory_free_raw_heap(%d): Failure to free raw heap. Parameter cannot be NULL.\n", __LINE__);

        return MEMORY_ERROR_NULL_PARAMETER;
    }

    free(*heap);

    *heap = NULL;

    return MEMORY_OK;
}

memory_error_code
memory_create_debug_context(u64 heapSafePtrRegionByteCapacity, u64 heapUserRegionByteCapacity, u64 heapLabelRegionByteCapacity, 
    const char *contextLabel, memory_short_id *outputMemoryDebugContextId)
{
    if (!outputMemoryDebugContextId)
    {
        fprintf(stderr, "memory_create_debug_context(%d): 'outputMemoryDebugContextId' parameter is NULL.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_PARAMETER;
    }

    u64

#ifndef MEMORY_DEBUG
    // check byte offset bounds
    {
        if ()
    }
#endif

    memory_error_code idGenResultCode;

    if ((idGenResultCode = _memory_generate_unique_short_id(&g_MEMORY_GLOBAL_SEED, outputMemoryDebugContextId)) != MEMORY_OK)
    {
        switch (idGenResultCode)
        {
            case MEMORY_ERROR_RANDOM_NOT_SEEDED:
            {
                fprintf(stderr, "memory_create_debug_context(%d): Error generating random ID. Did not provide a seed.\n", __LINE__);
                
                return MEMORY_ERROR_RANDOM_NOT_SEEDED;
            } break;

            default:
            {
                fprintf(stderr, "memory_create_debug_context(%d): Error generating random ID. Unknown.\n", __LINE__);

                return MEMORY_ERROR_UNKNOWN;
            } break;
        }
    }

    struct memory_debug_context *debugContextPtr;

    memory_error_code contextAllocResultCode;

    if ((contextAllocResultCode = _memory_alloc_context((struct memory_context **)&debugContextPtr, B32_TRUE)) != MEMORY_OK)
    {
        switch (contextAllocResultCode)
        {
            case MEMORY_ERROR_NULL_ID:
            {
                fprintf(stderr, "memory_create_debug_context(%d): Output parameter is NULL for internal '_memory_alloc_context' function result.\n",
                     __LINE__);

                return MEMORY_ERROR_FAILED_ALLOCATION;
            } break;

            default:
            {
                fprintf(stderr, "memory_create_debug_context(%d): Unknown error for internal '_memory_alloc_context' function result.\n",
                     __LINE__);
                return MEMORY_ERROR_UNKNOWN;
            } break;
        }
    }

    debugContextPtr->_.id = *outputMemoryDebugContextId;
    debugContextPtr->_.freeList = NULL;
    debugContextPtr->_.tailFree = NULL;
    debugContextPtr->_.bytesCapacity = labelRegionByteCapacity + userRegionByteCapacity + safePtrRegionByteCapacity;
    debugContextPtr->_.heap = heap + heapByteOffset;

    debugContextPtr->eventQueueCapacity = 0;
    debugContextPtr->eventQueueReadIndex = 0;
    debugContextPtr->eventQueueWriteIndex = 0;

    if (label)
    {
        strncpy(debugContextPtr->label, label, MEMORY_MAX_NAME_LENGTH);
    }
    else 
    {
        debugContextPtr->label[0] = '\0';
    }

    debugContextPtr->labelRegionByteOffset = labelRegionByteOffset;
    debugContextPtr->labelRegionByteCapacity = labelRegionByteCapacity;
    
    debugContextPtr->safePtrRegionByteOffset = safePtrRegionByteOffset;
    debugContextPtr->safePtrRegionByteCapacity = safePtrRegionByteCapacity;
    
    debugContextPtr->userRegionByteOffset = userRegionByteOffset;
    debugContextPtr->userRegionByteCapacity = userRegionByteCapacity;

    return MEMORY_OK;
}

memory_error_code
memory_create_context(u64 heapByteCapacity, u64 heapUserRegionByteCapacity, u64 heapUserRegionByteSize, 
    memory_short_id *outputMemoryContextId)
{
    if (!heap)
    {
        fprintf(stderr, "memory_create_context(%d): Provided heap cannot be NULL.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_PARAMETER;
    }
    
    if (!outputMemoryContextId)
    {
        fprintf(stderr, "memory_create_context(%d): Output parameter is NULL.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_PARAMETER;
    }
    
    memory_error_code idGenResultCode;

    if ((idGenResultCode = _memory_generate_unique_short_id(&g_MEMORY_GLOBAL_SEED, outputMemoryContextId)) != MEMORY_OK)
    {
        switch (idGenResultCode)
        {
            case MEMORY_ERROR_RANDOM_NOT_SEEDED:
            {
                fprintf(stderr, "memory_create_debug_context(%d): Error generating random ID. Did not provide a seed.\n", __LINE__);
                
                return MEMORY_ERROR_RANDOM_NOT_SEEDED;
            } break;

            default:
            {
                fprintf(stderr, "memory_create_debug_context(%d): Error generating random ID. Unknown.\n", __LINE__);

                return MEMORY_ERROR_UNKNOWN;
            } break;
        }
    }
    
    struct memory_context *contextPtr;
    
    memory_error_code contextAllocResultCode;

    if ((contextAllocResultCode = _memory_alloc_context(&contextPtr, B32_FALSE)) != MEMORY_OK)
    {
        switch (contextAllocResultCode)
        {
            case MEMORY_ERROR_NULL_ID:
            {
                fprintf(stderr, "memory_create_context(%d): Output parameter is NULL for internal '_memory_alloc_context' function result.\n",
                     __LINE__);

                return MEMORY_ERROR_FAILED_ALLOCATION;
            } break;

            default:
            {
                fprintf(stderr, "memory_create_context(%d): Unknown error for internal '_memory_alloc_context' function result.\n",
                     __LINE__);
                return MEMORY_ERROR_UNKNOWN;
            } break;
        }
    }

    return MEMORY_OK;
}

memory_error_code
memory_alloc_page(memory_short_id memoryContextId, u64 byteSize, memory_byte_id *outputPageId)
{
    // TODO:

    return MEMORY_ERROR_NOT_IMPLEMENTED;
}