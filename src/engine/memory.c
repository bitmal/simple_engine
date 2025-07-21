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

struct memory_allocator
{
    u16 identifier;
    u64 byteSize;
    struct memory_allocator *prev;
    struct memory_allocator *next;
};

struct memory_raw_allocation_info
{
    u16 rawAllocIndex;
    struct memory_raw_alloc_info *prev;
    struct memory_raw_alloc_info *next;
    b32 isActive;
};

struct memory_raw_allocation_key
{
    memory_short_id rawAllocId;
    u16 rawAllocBranchIndex;
};

struct memory_allocation_info
{
    memory_int_id allocId;
    memory_int_id pageId;
    struct memory_allocator *allocatorPtr;
    struct memory_allocation_info *prev;
    struct memory_allocation_info *next;
    b32 isActive;
};

struct memory_allocation_key
{
    memory_int_id allocId;
    u32 allocInfoIndex;
};

// if ((id - 1) < 1); then return NULL Error : return index
struct memory_allocation_safe_ptr
{
    memory_int_id safePtrId;
    p64 allocByteOffset;
    u64 refCount;
    b32 isActive;
};

struct memory_page_header
{
    u16 identifier;
    u64 heapByteSize;
    u32 allocationActiveCount;
    u32 allocationFreeCount;
    u32 allocationCapacity;
    struct memory_allocation_info *allocationArr;
    struct memory_allocation_info *allocationActiveList;
    struct memory_allocation_info *allocationFreeList;
    u32 allocatorActiveCount;
    u32 allocatorFreeCount;
    struct memory_allocator *allocatorActiveList;
    struct memory_allocator *allocatorFreeList;
    struct memory_page_header *prev;
    struct memory_page_header *next;
};

// (pageId, allocationKey) -->
// page info by page index (id - 1; if not NULL ID) -->
//  page header (using pageHeaderByteOffset) -->
//      using allocation key -->
//      
struct memory_page_info
{
    u64 pageHeaderByteOffset;
    enum memory_page_status_t status;
};

struct memory_context
{
    memory_id id;
    p64 safePtrRegionByteOffset;
    u64 safePtrRegionByteCapacity;
    p64 pagesRegionByteOffset;
    u64 pagesRegionByteCapacity;
    struct memory_page_info *pagesRegionInfoArr;
    u16 pagesRegionInfoCount;
    u16 pagesRegionInfoCapacity;
    struct memory_page_header *activePages;
    u16 pagesRegionActiveCount;
    struct memory_page_header *freePages;
    u16 pagesRegionFreeCount;
    u32 pagesRegionActiveAllocationCount;
    u32 pagesRegionFreeAllocationCount;
    u8 *heap;
    b32 isDebug;
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
static struct memory_raw_alloc_info *g_MEMORY_RAW_ALLOC_INFO_ACTIVE_LIST;
static struct memory_raw_alloc_info *g_MEMORY_RAW_ALLOC_INFO_FREE_LIST;
static struct memory_debug_context *g_DEBUG_CONTEXT_ARR;
static struct memory_context *g_CONTEXT_ARR;
u16 g_DEBUG_CONTEXT_CAPACITY;
u16 g_DEBUG_CONTEXT_COUNT;
static struct memory_debug_context *g_DEBUG_CONTEXT_ARR;
u16 g_CONTEXT_CAPACITY;
u16 g_CONTEXT_COUNT;

static memory_error_code
_memory_generate_unique_byte_id(memory_byte_id *outResult)
{
    assert(outResult);

    memory_error_code errorResult;

    if ((utils_get_is_random_seed_set()))
    {
        real64 resultRandomNumber;

        if (utils_generate_random_positive_normalized_real64(&resultRandomNumber))
        {
            *outResult = (memory_byte_id)(round(resultRandomNumber*(UINT8_MAX - 1))) + 1;
            errorResult = MEMORY_OK;
        }
        else 
        {
            *outResult = MEMORY_BYTE_ID_NULL;
            errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
        }
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

    if ((utils_get_is_random_seed_set()))
    {
        real64 resultRandomNumber;
        
        if (utils_generate_random_positive_normalized_real64(&resultRandomNumber))
        {
            *outResult = (memory_short_id)(round(resultRandomNumber*(UINT16_MAX - 1))) + 1;
            errorResult = MEMORY_OK;
        }
        else 
        {
            *outResult = MEMORY_SHORT_ID_NULL;
            errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
        }
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

    if ((utils_get_is_random_seed_set()))
    {
        real64 resultRandomNumber;
        
        if (utils_generate_random_positive_normalized_real64(&resultRandomNumber))
        {
            *outResult = (memory_int_id)(round(resultRandomNumber*(UINT32_MAX - 1))) + 1;
            errorResult = MEMORY_OK;
        }
        else 
        {
            *outResult = MEMORY_INT_ID_NULL;
            errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
        }
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

    if ((utils_get_is_random_seed_set()))
    {
        u64 resultRandomNumber;
        
        if (utils_generate_random_u64(&resultRandomNumber))
        {
            *outResult = (resultRandomNumber < MEMORY_ID_MAX) ? (resultRandomNumber + 1) : resultRandomNumber;
            errorResult = MEMORY_OK;
        }
        else 
        {
            *outResult = MEMORY_INT_ID_NULL;
            errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
        }
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
    // TODO:
    static b32 isInit = B32_FALSE;

    if (!isInit)
    {
        isInit = B32_TRUE;
    }

    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

static memory_error_code
_memory_alloc_context(struct memory_context **outMemoryContext, b32 isDebug)
{
    if (!outMemoryContext)
    {
        return MEMORY_ERROR_NULL_PARAMETER;
    }

    #define MEMORY_DEBUG_CONTEXT_ARR_REALLOC_MULTIPLIER 2

    if (isDebug)
    {
        u16 debugContextIndex = g_DEBUG_CONTEXT_COUNT;

        if (g_DEBUG_CONTEXT_COUNT == g_DEBUG_CONTEXT_CAPACITY)
        {
            if (g_DEBUG_CONTEXT_CAPACITY > 0)
            {
                struct memory_debug_context *reallocatedMemoryContext =
                    realloc(g_DEBUG_CONTEXT_ARR, sizeof(struct memory_debug_context)*(g_DEBUG_CONTEXT_CAPACITY*
                        MEMORY_DEBUG_CONTEXT_ARR_REALLOC_MULTIPLIER));

                if (reallocatedMemoryContext)
                {
                    g_DEBUG_CONTEXT_ARR = reallocatedMemoryContext;
                    g_DEBUG_CONTEXT_CAPACITY *= MEMORY_DEBUG_CONTEXT_ARR_REALLOC_MULTIPLIER;
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
                g_DEBUG_CONTEXT_ARR = malloc(sizeof(struct memory_debug_context));

                if (g_DEBUG_CONTEXT_ARR)
                {
                    ++g_DEBUG_CONTEXT_CAPACITY;
                }
                else 
                {
                    fprintf(stderr, "_memory_alloc_context(%d): Failure to allocate debug context array. "
                            "Cannot allocate new debug context.\n", __LINE__);

                    return MEMORY_ERROR_FAILED_ALLOCATION;
                }
            }

        }

        *outMemoryContext = (struct memory_context *)&g_DEBUG_CONTEXT_ARR[debugContextIndex];
        ++g_DEBUG_CONTEXT_COUNT;
    }
    else 
    {
        #define MEMORY_CONTEXT_ARR_REALLOC_MULTIPLIER 2

        u16 contextIndex = g_CONTEXT_COUNT;

        if (g_CONTEXT_COUNT == g_CONTEXT_CAPACITY)
        {
            if (g_CONTEXT_CAPACITY > 0)
            {
                struct memory_context *reallocatedMemoryContext =
                    realloc(g_CONTEXT_ARR, sizeof(struct memory_context)*(g_CONTEXT_CAPACITY*
                        MEMORY_CONTEXT_ARR_REALLOC_MULTIPLIER));

                if (reallocatedMemoryContext)
                {
                    g_CONTEXT_ARR = reallocatedMemoryContext;
                    g_CONTEXT_CAPACITY *= MEMORY_CONTEXT_ARR_REALLOC_MULTIPLIER;
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
                g_CONTEXT_ARR = malloc(sizeof(struct memory_context));

                if (g_CONTEXT_ARR)
                {
                    ++g_CONTEXT_CAPACITY;
                }
                else 
                {
                    fprintf(stderr, "_memory_alloc_context(%d): Failure to allocate context array. "
                            "Cannot allocate new context.\n", __LINE__);

                    return MEMORY_ERROR_FAILED_ALLOCATION;
                }
            }
        }

        *outMemoryContext = (struct memory_context *)&g_CONTEXT_ARR[contextIndex];
        ++g_CONTEXT_COUNT;
    }
    
    return MEMORY_OK;
}

memory_error_code
memory_alloc_raw_allocation(const struct memory_raw_allocation_key *outRawAllocKeyPtr, u64 byteSize)
{
    {
        b32 isOutNull = (!outRawAllocKeyPtr);
        b32 isByteSizeLessThanOne = (byteSize < 1);
        b32 isRawAllocsUnderMaxCount = (g_MEMORY_RAW_ALLOC_COUNT < MEMORY_MAX_RAW_ALLOCS);
        
        if (!isOutNull || isByteSizeLessThanOne || !isRawAllocsUnderMaxCount)
        {
            memory_error_code resultCode;

            if (!isOutNull)
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

    // initialize

    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

memory_error_code
memory_realloc_raw_allocation(const struct memory_raw_allocation_key *rawAllocKeyPtr, u64 byteSize)
{
    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

memory_error_code
memory_free_raw_allocation(const struct memory_raw_allocation_key *outRawAllocKeyPtr)
{
    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

memory_error_code
memory_map_raw_allocation(const struct memory_raw_allocation_key *rawAllocKeyPtr, void **outDataPtr)
{
    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

memory_error_code
memory_unmap_raw_allocation(const struct memory_raw_allocation_key *rawAllocKeyPtr, void **outDataPtr)
{
    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

memory_error_code
memory_create_debug_context(u64 safePtrRegionByteCapacity, u64 pagesRegionByteCapacity, u64 labelRegionByteCapacity, 
    const char *contextLabel, memory_short_id *outputMemoryDebugContextId)
{
    if (!outputMemoryDebugContextId)
    {
        fprintf(stderr, "memory_create_debug_context(%d): 'outputMemoryDebugContextId' parameter is NULL.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_PARAMETER;
    }

    u64 totalBytesToAllocate = safePtrRegionByteCapacity + pagesRegionByteCapacity + labelRegionByteCapacity;

#ifndef MEMORY_DEBUG
    // check byte offset bounds
    {
    }
#endif

    memory_error_code idGenResultCode;

    if ((idGenResultCode = _memory_generate_unique_short_id(outputMemoryDebugContextId)) != MEMORY_OK)
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
    debugContextPtr->_.pagesRegionActiveAllocationCount = 0;
    debugContextPtr->_.pagesRegionFreeAllocationCount = 0;
    debugContextPtr->_.pagesRegionByteCapacity = pagesRegionByteCapacity;
    debugContextPtr->_.pagesRegionByteOffset = (p64)(safePtrRegionByteCapacity);
    debugContextPtr->_.pagesRegionInfoArr = NULL;
    debugContextPtr->_.safePtrRegionByteCapacity = safePtrRegionByteCapacity;
    debugContextPtr->_.safePtrRegionByteOffset = 0;
    debugContextPtr->_.heap = malloc(totalBytesToAllocate);
    debugContextPtr->_.pagesRegionInfoCapacity = 0;
    debugContextPtr->_.pagesRegionInfoCount = 0;
    debugContextPtr->_.isDebug = B32_TRUE;

    debugContextPtr->eventQueueCapacity = 0;
    debugContextPtr->eventQueueReadIndex = 0;
    debugContextPtr->eventQueueWriteIndex = 0;

    if (contextLabel)
    {
        strncpy(debugContextPtr->label, contextLabel, MEMORY_MAX_NAME_LENGTH);
    }
    else 
    {
        debugContextPtr->label[0] = '\0';
    }

    debugContextPtr->eventQueueCapacity = 0;
    debugContextPtr->eventQueueReadIndex = 0;
    debugContextPtr->eventQueueWriteIndex = 0;
    debugContextPtr->labelRegionByteCapacity = labelRegionByteCapacity;
    debugContextPtr->labelRegionByteOffset = safePtrRegionByteCapacity;

    return MEMORY_OK;
}

memory_error_code
memory_create_context(u64 safePtrRegionByteCapacity, u64 pagesRegionByteCapacity, memory_short_id *outputMemoryContextId)
{
    if (!outputMemoryContextId)
    {
        fprintf(stderr, "memory_create_debug_context(%d): 'outputMemoryDebugContextId' parameter is NULL.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_PARAMETER;
    }

    u64 totalBytesToAllocate = safePtrRegionByteCapacity + pagesRegionByteCapacity;
    
#ifndef MEMORY_DEBUG
    // check byte offset bounds
    {
    }
#endif

    memory_error_code idGenResultCode;

    if ((idGenResultCode = _memory_generate_unique_short_id(outputMemoryContextId)) != MEMORY_OK)
    {
        switch (idGenResultCode)
        {
            case MEMORY_ERROR_RANDOM_NOT_SEEDED:
            {
                fprintf(stderr, "memory_create_context(%d): Error generating random ID. Did not provide a seed.\n", __LINE__);
                
                return MEMORY_ERROR_RANDOM_NOT_SEEDED;
            } break;

            default:
            {
                fprintf(stderr, "memory_create_context(%d): Error generating random ID. Unknown.\n", __LINE__);

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

    contextPtr->id = *outputMemoryContextId;
    contextPtr->pagesRegionActiveAllocationCount = 0;
    contextPtr->pagesRegionFreeAllocationCount = 0;
    contextPtr->pagesRegionByteCapacity = pagesRegionByteCapacity;
    contextPtr->pagesRegionByteOffset = (p64)(safePtrRegionByteCapacity);
    contextPtr->pagesRegionInfoArr = NULL;
    contextPtr->safePtrRegionByteCapacity = safePtrRegionByteCapacity;
    contextPtr->safePtrRegionByteOffset = 0;
    contextPtr->heap = malloc(totalBytesToAllocate);
    contextPtr->pagesRegionInfoCapacity = 0;
    contextPtr->pagesRegionInfoCount = 0;
    contextPtr->isDebug = B32_FALSE;

    return MEMORY_OK;
}

memory_error_code
memory_alloc_page(memory_short_id memoryContextId, u64 byteSize, memory_short_id *outPageIdPtr)
{
    // TODO: check bounds and pointers for NULL

    // get a free page or allocate one
    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

memory_error_code
memory_free_page(memory_short_id memoryContextId, memory_short_id pageId)
{
    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

memory_error_code
memory_realloc_page(memory_short_id memoryContextId, memory_short_id pageId)
{
    return MEMORY_ERROR_NOT_IMPLEMENTED;
}