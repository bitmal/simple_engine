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
    u64 allocCount;
};

struct memory_safe_ptr
{
    memory_id allocId;
    memory_id pageId;
    p64 allocByteOffset;
    u64 refCount;
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
    u64 bytesCapacity;
    struct memory_allocator *freeList;
    struct memory_allocator *tailFree;
    u64 userRegionPageByteOffsets[MEMORY_MAX_USER_REGION_PAGES];
    u8 *heap;
};

struct memory_debug_context
{
    struct memory_context _;

    char label[MEMORY_MAX_NAME_LENGTH + 1];
    p64 labelRegionByteOffset;
    u64 labelRegionByteCapacity;
    p64 safePtrRegionByteOffset;
    u64 safePtrRegionByteCapacity;
    p64 userRegionByteOffset;
    u64 userRegionByteCapacity;
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

static i64 g_MEMORY_GLOBAL_SEED;

void
memory_set_global_seed(i64 seed)
{
    g_MEMORY_GLOBAL_SEED = seed;
}

i64
memory_get_global_seed()
{
    return g_MEMORY_GLOBAL_SEED;
}

static memory_error_code
_memory_generate_unique_byte_id(i64 *seedPtr, memory_byte_id *outResult)
{
    assert(outResult);
    
    static b32 isSeeded = B32_FALSE;

    if (seedPtr)
    {
        isSeeded = B32_TRUE;
    }
    
    memory_error_code errorResult;

    if (isSeeded)
    {
        *outResult = (memory_byte_id)(round(UTILS_GENERATE_RANDOM_POSITIVE_REAL64(seedPtr)*UINT8_MAX));
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
_memory_generate_unique_short_id(i64 *seedPtr, memory_short_id *outResult)
{
    assert(outResult);
    
    static b32 isSeeded = B32_FALSE;

    if (seedPtr)
    {
        isSeeded = B32_TRUE;
    }
    
    memory_error_code errorResult;

    if (isSeeded)
    {
        *outResult = (memory_short_id)(round(UTILS_GENERATE_RANDOM_POSITIVE_REAL64(seedPtr)*UINT16_MAX));
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
_memory_generate_unique_int_id(i64 *seedPtr, memory_int_id *outResult)
{
    assert(outResult);
    
    static b32 isSeeded = B32_FALSE;

    if (seedPtr)
    {
        isSeeded = B32_TRUE;
    }
    
    memory_error_code errorResult;

    if (isSeeded)
    {
        *outResult = (memory_int_id)(round(UTILS_GENERATE_RANDOM_POSITIVE_REAL64(seedPtr)*UINT32_MAX));
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
_memory_generate_unique_id(u16 *seedPtr, memory_id *outResult)
{
    assert(outResult);
    
    static b32 isSeeded = B32_FALSE;

    if (seedPtr)
    {
        isSeeded = B32_TRUE;
    }
    
    memory_error_code errorResult;

    if (isSeeded)
    {
        *outResult = (memory_id)(utils_generate_random_u64(seedPtr));
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
_memory_alloc_context(struct memory_context **outMemoryContext, b32 isDebug)
{
    if (!outMemoryContext)
    {
        return MEMORY_ERROR_NULL_PARAMETER;
    }
    
    static struct memory_context *memoryContextArr = NULL;
    static u16 memoryContextCount = 0;
    
    static struct memory_debug_context *memoryDebugContextArr = NULL;
    static u16 memoryDebugContextCount = 0;

    u16 totalMemoryContexts = memoryContextCount + memoryDebugContextCount;

    struct memory_context *memoryContextPtr;

    if (isDebug)
    {
        u16 index = memoryDebugContextCount;
        {
            memory_error_code allocationResultCode;

            if (memoryDebugContextCount > 0)
            {
                allocationResultCode = memory_realloc_raw_heap((u8 **)&memoryDebugContextArr, sizeof(struct memory_debug_context)*(++memoryDebugContextCount));
            }
            else 
            {
                allocationResultCode = memory_alloc_raw_heap((u8 **)&memoryDebugContextArr, sizeof(struct memory_debug_context)*(++memoryDebugContextCount));
            }

            if (allocationResultCode != MEMORY_OK)
            {
                return allocationResultCode;
            }
        }

        memoryContextPtr = (struct memory_context *)&memoryDebugContextArr[index];
    }
    else 
    {
        u16 index = memoryContextCount;
        {
            memory_error_code allocationResultCode;

            if (memoryContextCount > 0)
            {
                allocationResultCode = memory_realloc_raw_heap((u8 **)&memoryContextArr, sizeof(struct memory_context)*(++memoryContextCount));
            }
            else 
            {
                allocationResultCode = memory_alloc_raw_heap((u8 **)&memoryContextArr, sizeof(struct memory_context)*(++memoryContextCount));
            }

            if (allocationResultCode != MEMORY_OK)
            {
                return allocationResultCode;
            }
        }        

        memoryContextPtr = (struct memory_context *)&memoryContextArr[index];
    }

    *outMemoryContext = memoryContextPtr;

    if (isDebug)
    {
        ++memoryDebugContextCount;
    }
    else
    {
        ++memoryContextCount;
    }

    return MEMORY_OK;
}

memory_error_code
memory_alloc_raw_heap(u8 **outResult, u64 byteSize)
{
    if (!outResult)
    {
        fprintf(stderr, "memory_alloc_raw_heap(%d): Out parameter cannot be NULL.\n", __LINE__);

        return MEMORY_ERROR_NULL_PARAMETER;
    }

    if (byteSize < 1)
    {
        fprintf(stderr, "memory_alloc_raw_heap(%d): ByteSize < 1. Failure to allocate.\n", __LINE__);
        
        return MEMORY_ERROR_ZERO_PARAMETER;
    }

    *outResult = malloc(byteSize);

    if (!outResult)
    {
        fprintf(stderr, "memory_alloc_raw_heap(%d): Failure to allocate raw heap.\n", __LINE__);

        return MEMORY_ERROR_FAILED_ALLOCATION;
    }

    return MEMORY_OK;
}

memory_error_code
memory_realloc_raw_heap(u8 **outResult, u64 byteSize)
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
memory_create_debug_context(u8 *heap, const char *label, p64 heapByteOffset, p64 labelRegionByteOffset, u64 labelRegionByteCapacity, 
    p64 safePtrRegionByteOffset, u64 safePtrRegionByteCapacity, p64 userRegionByteOffset, u64 userRegionByteCapacity, memory_short_id *outputMemoryDebugContextId)
{
    if (!heap)
    {
        fprintf(stderr, "memory_create_debug_context(%d): Provided heap cannot be NULL.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_PARAMETER;
    }
    
    if (!outputMemoryDebugContextId)
    {
        fprintf(stderr, "memory_create_debug_context(%d): Output parameter is NULL.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_PARAMETER;
    }

#ifdef MEMORY_DEBUG
    // check byte offset bounds
    {
        p64 maxLabelRegionIndex = labelRegionByteOffset + labelRegionByteCapacity - 1;
        p64 maxSafePtrRegionIndex = safePtrRegionByteOffset + safePtrRegionByteCapacity - 1;
        p64 maxUserRegionIndex = userRegionByteOffset + userRegionByteCapacity - 1;
        
        if ((labelRegionByteOffset >= safePtrRegionByteOffset) || (labelRegionByteOffset >= userRegionByteOffset) ||
            (labelRegionByteOffset <= maxSafePtrRegionIndex) || (labelRegionByteOffset <= maxUserRegionIndex) ||
            (maxLabelRegionIndex >= safePtrRegionByteOffset) || (maxLabelRegionIndex <= maxSafePtrRegionIndex) ||
            (safePtrRegionByteOffset >= userRegionByteOffset) || (safePtrRegionByteOffset <= maxUserRegionIndex) ||
            (maxSafePtrRegionIndex <= maxUserRegionIndex))
        {
            fprintf(stderr, "memory_create_debug_context(%d): Heap regions out of byte index range.\n", __LINE__);
            
            return MEMORY_ERROR_INDEX_OUT_OF_RANGE;
        }
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
memory_create_context(u8 *heap, p64 heapOffset, u64 heapCapacity, memory_short_id *outputMemoryContextId)
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