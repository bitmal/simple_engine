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

#define MEMORY_HEADER_ID ((u16)0xDEAD)

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
    memory_short_id pageId;
    struct memory_allocator *allocatorPtr;
    struct memory_allocation_info *prev;
    struct memory_allocation_info *next;
    b32 isActive;
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
    u64 reservedHeapBytes;
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
static u16 g_DEBUG_CONTEXT_CAPACITY;
static u16 g_DEBUG_CONTEXT_COUNT;
static struct memory_debug_context *g_DEBUG_CONTEXT_ARR;
static u16 g_CONTEXT_CAPACITY;
static u16 g_CONTEXT_COUNT;

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
            *outResult = MEMORY_ID_NULL;
            errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
        }
    }
    else
    {
        *outResult = MEMORY_ID_NULL;
        errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
    }

   return errorResult;
}

static memory_error_code
_memory_alloc_context(struct memory_context **outMemoryContext, b32 isDebug)
{
    if (!outMemoryContext)
    {
        return MEMORY_ERROR_NULL_ARGUMENT;
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
_memory_get_context(struct memory_context_key *contextKeyPtr, struct memory_context **outContextPtr)
{
    if (!contextKeyPtr)
    {
        fprintf(stderr, "_memory_get_context(%d): 'memoryContextKeyPtr' parameter is NULL. "
                "Cannot find context.\n", __LINE__);

        if (outContextPtr)
        {
            *outContextPtr = NULL;
        }
                
        return MEMORY_ERROR_NULL_ARGUMENT;
    }
    
    if (!outContextPtr)
    {
        fprintf(stderr, "_memory_get_context(%d): Output parameter is NULL. "
                "Cannot get context.\n", __LINE__);
                
        return MEMORY_ERROR_NULL_ARGUMENT;
    }

    struct memory_context *contextPtr;
    {
        memory_error_code contextSearchResultCode;
        
        if (!contextKeyPtr->isDebug)
        {
            if (contextKeyPtr->contextId > 0)
            {
                u16 contextIndex = contextKeyPtr->contextId - 1;
                
                if (contextIndex < g_CONTEXT_COUNT)
                {
                    contextPtr = &g_CONTEXT_ARR[contextKeyPtr->contextId - 1];
                    contextSearchResultCode = MEMORY_OK;
                }
                else 
                {
                    contextSearchResultCode = MEMORY_ERROR_INDEX_OUT_OF_RANGE;
                }
            }
            else 
            {
                contextSearchResultCode = MEMORY_ERROR_NULL_ID;
            }
        }
        else 
        {
            if (contextKeyPtr->contextId > 0)
            {
                u16 contextIndex = contextKeyPtr->contextId - 1;
                
                if (contextIndex < g_DEBUG_CONTEXT_COUNT)
                {
                    contextPtr = (struct memory_context *)&g_DEBUG_CONTEXT_ARR[contextKeyPtr->contextId - 1];
                    contextSearchResultCode = MEMORY_OK;
                }
                else 
                {
                    contextSearchResultCode = MEMORY_ERROR_INDEX_OUT_OF_RANGE;
                }
            }
            else 
            {
                contextSearchResultCode = MEMORY_ERROR_NULL_ID;
            }
        }
        
        if (contextSearchResultCode != MEMORY_OK)
        {
            *outContextPtr = NULL;
            
            fprintf(stderr, "memory_alloc_page(%d): Failure to identiy memory context from key. "
                    "Cannot allocate a page.\n", __LINE__);
                    
            return contextSearchResultCode;
        }
    }

    *outContextPtr = contextPtr;
    
    return MEMORY_OK;
}

memory_error_code
_memory_get_context_key_is_ok(const struct memory_context_key *contextKeyPtr)
{
    if (!contextKeyPtr)
    {
        return MEMORY_ERROR_NULL_ARGUMENT;
    }

    if (contextKeyPtr->contextId == MEMORY_SHORT_ID_NULL)
    {
        return MEMORY_ERROR_NULL_ID;
    }

    return MEMORY_OK;
}

memory_error_code
_memory_get_page_key_is_ok(const struct memory_page_key *pageKeyPtr)
{
    if (!pageKeyPtr)
    {
        return MEMORY_ERROR_NULL_ARGUMENT;
    }

    if (pageKeyPtr->pageId == MEMORY_SHORT_ID_NULL)
    {
        return MEMORY_ERROR_NULL_ID;
    }

    if ((_memory_get_context_key_is_ok(&pageKeyPtr->contextKey)) != MEMORY_OK)
    {
        return MEMORY_ERROR_NOT_AN_ACTIVE_CONTEXT;
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

                resultCode = MEMORY_ERROR_NULL_ARGUMENT;
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
    const char *contextLabel, const struct memory_context_key *outputMemoryDebugContextKeyPtr)
{
    {
        memory_error_code errorCode;

        if ((errorCode = _memory_get_context_key_is_ok(outputMemoryDebugContextKeyPtr)) != MEMORY_OK)
        {
            fprintf(stderr, "memory_create_debug_context(%d): 'outputMemoryDebugContextKeyPtr' parameter is NULL.\n", __LINE__);
            
            return MEMORY_ERROR_NULL_ARGUMENT;
        }
    }

    u64 totalBytesToAllocate = safePtrRegionByteCapacity + pagesRegionByteCapacity + labelRegionByteCapacity;

    struct memory_debug_context *debugContextPtr;
    memory_short_id contextId = g_DEBUG_CONTEXT_COUNT;
    {
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
    }

    debugContextPtr->_.id = contextId;
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

    ((struct memory_context_key *)outputMemoryDebugContextKeyPtr)->contextId = contextId;
    ((struct memory_context_key *)outputMemoryDebugContextKeyPtr)->isDebug = B32_TRUE;

    return MEMORY_OK;
}

memory_error_code
memory_create_context(u64 safePtrRegionByteCapacity, u64 pagesRegionByteCapacity, 
    const struct memory_context_key *outputMemoryContextKeyPtr)
{
    if (!outputMemoryContextKeyPtr)
    {
        fprintf(stderr, "memory_create_context(%d): 'outputMemoryContextKeyPtr' parameter is NULL.\n", __LINE__);

        return MEMORY_ERROR_NULL_ARGUMENT;
    }

    u64 totalBytesToAllocate = safePtrRegionByteCapacity + pagesRegionByteCapacity;

    struct memory_context *contextPtr;
    memory_short_id contextId = g_CONTEXT_COUNT;
    {
        memory_error_code contextAllocResultCode;

        if ((contextAllocResultCode = _memory_alloc_context(&contextPtr, B32_FALSE)) != MEMORY_OK)
        {
            switch (contextAllocResultCode)
            {
                case MEMORY_ERROR_NULL_ID:
                {
                    fprintf(stderr, "memory_create_context(%d): Output parameter is NULL for internal '_memory_alloc_context' function result.\n",
                         __LINE__);
                    
                    memset((struct memory_context_key *)outputMemoryContextKeyPtr, '\0', sizeof(struct memory_context_key));

                    return MEMORY_ERROR_FAILED_ALLOCATION;
                } break;

                default:
                {
                    fprintf(stderr, "memory_create_context(%d): Unknown error for internal '_memory_alloc_context' function result.\n",
                         __LINE__);
                    
                    memset((struct memory_context_key *)outputMemoryContextKeyPtr, '\0', sizeof(struct memory_context_key));

                    return MEMORY_ERROR_UNKNOWN;
                } break;
            }
        }
    }

    contextPtr->id = contextId;
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
    
    ((struct memory_context_key *)outputMemoryContextKeyPtr)->contextId = contextId;
    ((struct memory_context_key *)outputMemoryContextKeyPtr)->isDebug = B32_FALSE;

    return MEMORY_OK;
}

memory_error_code
memory_get_context_key_is_ok(const struct memory_context_key *contextKeyPtr)
{
    return _memory_get_context_key_is_ok(contextKeyPtr);
}

memory_error_code
memory_alloc_page(const struct memory_context_key *memoryContextKeyPtr, u64 byteSize, const struct memory_page_key *outPageKeyPtr)
{
    {
        memory_error_code resultCode;

        if ((resultCode = _memory_get_context_key_is_ok(memoryContextKeyPtr)) != MEMORY_OK)
        {
            switch (resultCode)
            {
                default: 
                {
                    fprintf(stderr, "%s(%d): Cannot allocate page, context key is not valid!\n", 
                            __FUNCTION__, __LINE__);
                } return resultCode;
            }
            
            if ((resultCode = _memory_get_page_key_is_ok(outPageKeyPtr)) != MEMORY_OK)
            {
                ((struct memory_page_key *)outPageKeyPtr)->pageId = MEMORY_SHORT_ID_NULL;
                ((struct memory_page_key *)outPageKeyPtr)->contextKey.contextId = MEMORY_SHORT_ID_NULL;
                ((struct memory_page_key *)outPageKeyPtr)->contextKey.isDebug = B32_FALSE;
            }
                    
            return MEMORY_ERROR_NULL_ARGUMENT;
        }
    }
    
    if (!outPageKeyPtr)
    {
        fprintf(stderr, "memory_alloc_page(%d): Output parameter is NULL. "
                "Cannot allocate a page.\n", __LINE__);

        return MEMORY_ERROR_NULL_ARGUMENT;
    }

    struct memory_context *contextPtr;
    {
        memory_error_code resultCode;

        if ((resultCode =_memory_get_context((struct memory_context_key *)memoryContextKeyPtr, 
            &contextPtr)) != MEMORY_OK)
        {
            ((struct memory_page_key *)outPageKeyPtr)->pageId = MEMORY_SHORT_ID_NULL;
            ((struct memory_page_key *)outPageKeyPtr)->contextKey.contextId = MEMORY_SHORT_ID_NULL;
            ((struct memory_page_key *)outPageKeyPtr)->contextKey.isDebug = B32_FALSE;

            return resultCode;
        }
    }

    // get/allocate a page header and allocated bytes
    struct memory_page_header *pageHeaderPtr;

    if (contextPtr->pagesRegionFreeCount > 0)
    {
        pageHeaderPtr = contextPtr->freePages;

        memory_error_code resultCode = MEMORY_OK;

        do
        {
            if (pageHeaderPtr->heapByteSize >= byteSize)
            {
                break;
            }
        } while((pageHeaderPtr = pageHeaderPtr->next) != contextPtr->freePages);

        if (resultCode != MEMORY_OK)
        {
            ((struct memory_page_key *)outPageKeyPtr)->pageId = MEMORY_SHORT_ID_NULL;
            ((struct memory_page_key *)outPageKeyPtr)->contextKey.contextId = MEMORY_SHORT_ID_NULL;
            ((struct memory_page_key *)outPageKeyPtr)->contextKey.isDebug = B32_FALSE;
            
            fprintf(stderr, "memory_alloc_page(%d): Failure to locate page large enough. "
                    "Cannot allocate a page.\n", __LINE__);

            return resultCode;
        }

        --contextPtr->pagesRegionFreeCount;
    }
    // if there are no free pages, and there are no active pages, then there are zero allocated pages! 
    // Therefore allocate the first one.
    else if (contextPtr->pagesRegionActiveCount < 1)
    {
        pageHeaderPtr = (struct memory_page_header *)(contextPtr->heap + contextPtr->pagesRegionByteOffset);

        memset(pageHeaderPtr, '\0', sizeof(struct memory_page_header));

        pageHeaderPtr->identifier = MEMORY_HEADER_ID;
    }
    else 
    {
        ((struct memory_page_key *)outPageKeyPtr)->pageId = MEMORY_SHORT_ID_NULL;
        ((struct memory_page_key *)outPageKeyPtr)->contextKey.contextId = MEMORY_SHORT_ID_NULL;
        ((struct memory_page_key *)outPageKeyPtr)->contextKey.isDebug = B32_FALSE;

        fprintf(stderr, "memory_alloc_page(%d): Out of space. "
                "Cannot allocate a page.\n", __LINE__);

        return MEMORY_ERROR_FAILED_ALLOCATION;
    }

    if (contextPtr->pagesRegionActiveCount > 0)
    {
        pageHeaderPtr->next = contextPtr->activePages;
        pageHeaderPtr->prev = contextPtr->activePages->prev;

        contextPtr->activePages->prev->next = pageHeaderPtr;
        contextPtr->activePages->prev = pageHeaderPtr;
    }
    else 
    {
        pageHeaderPtr->prev = pageHeaderPtr->next = pageHeaderPtr;
    }

    pageHeaderPtr->identifier = MEMORY_HEADER_ID;

    contextPtr->activePages = pageHeaderPtr;
    ++contextPtr->pagesRegionActiveCount;

    // split the page for future page allocations, if we can
    u64 diffByteSize = pageHeaderPtr->heapByteSize - byteSize;
    
    if (diffByteSize > sizeof(struct memory_page_header))
    {
        p64 splitPageHeaderByteOffset = contextPtr->pagesRegionByteOffset + sizeof(struct memory_page_header) + pageHeaderPtr->heapByteSize;
        struct memory_page_header *splitPageHeader = (struct memory_page_header *)(contextPtr->heap + splitPageHeaderByteOffset);

        memset(splitPageHeader, '\0', sizeof(struct memory_page_header));

        splitPageHeader->identifier = MEMORY_HEADER_ID;
        splitPageHeader->heapByteSize = diffByteSize - sizeof(struct memory_page_header);
        
        pageHeaderPtr->heapByteSize = byteSize;

        if (contextPtr->pagesRegionFreeCount > 0)
        {
            contextPtr->freePages->prev->next = splitPageHeader;

            splitPageHeader->prev = contextPtr->freePages->prev;
            contextPtr->freePages->prev = splitPageHeader;

            splitPageHeader->next = contextPtr->freePages;
        }
        else 
        {
            splitPageHeader->prev = splitPageHeader->next = splitPageHeader;
        }
        
        contextPtr->freePages = splitPageHeader;
        ++contextPtr->pagesRegionFreeCount;
    }
    else 
    {
        pageHeaderPtr->heapByteSize = contextPtr->pagesRegionByteCapacity - sizeof(struct memory_page_header);
    }

    #define PAGES_REGION_INFO_ARR_REALLOC_MULTIPLIER 4

    memory_short_id pageId = contextPtr->pagesRegionInfoCount + 1;
    struct memory_page_info *pageInfoPtr;

    if (contextPtr->pagesRegionInfoCount != contextPtr->pagesRegionInfoCapacity)
    {
        pageInfoPtr = &contextPtr->pagesRegionInfoArr[pageId - 1];
    }
    else
    {
        if (contextPtr->pagesRegionInfoCount > 0)
        {
            struct memory_page_info *tempPtr = realloc(contextPtr->pagesRegionInfoArr, sizeof(struct memory_page_info)*
                (contextPtr->pagesRegionInfoCapacity*PAGES_REGION_INFO_ARR_REALLOC_MULTIPLIER));
            
            if (tempPtr)
            {
                contextPtr->pagesRegionInfoArr = tempPtr;
                pageInfoPtr = &tempPtr[pageId - 1];

                contextPtr->pagesRegionInfoCapacity *= PAGES_REGION_INFO_ARR_REALLOC_MULTIPLIER;
            }
            else 
            {
                ((struct memory_page_key *)outPageKeyPtr)->pageId = MEMORY_SHORT_ID_NULL;
                ((struct memory_page_key *)outPageKeyPtr)->contextKey.contextId = MEMORY_SHORT_ID_NULL;
                ((struct memory_page_key *)outPageKeyPtr)->contextKey.isDebug = B32_FALSE;

                fprintf(stderr, "memory_alloc_page(%d): Could not reallocate the pages info array. "
                        "Cannot allocate a page.\n", __LINE__);
                        
                return MEMORY_ERROR_FAILED_ALLOCATION;
            }
        }
        else 
        {
            contextPtr->pagesRegionInfoArr = malloc(sizeof(struct memory_page_info));

            if (contextPtr->pagesRegionInfoArr)
            {
                pageInfoPtr = &contextPtr->pagesRegionInfoArr[pageId - 1];

                ++contextPtr->pagesRegionInfoCapacity;
            }
            else 
            {
                ((struct memory_page_key *)outPageKeyPtr)->pageId = MEMORY_SHORT_ID_NULL;
                ((struct memory_page_key *)outPageKeyPtr)->contextKey.contextId = MEMORY_SHORT_ID_NULL;
                ((struct memory_page_key *)outPageKeyPtr)->contextKey.isDebug = B32_FALSE;

                fprintf(stderr, "memory_alloc_page(%d): Could not allocate the pages info array. "
                        "Cannot allocate a page.\n", __LINE__);
                        
                return MEMORY_ERROR_FAILED_ALLOCATION;
            }
        }
    }

    ++contextPtr->pagesRegionInfoCount;

    pageInfoPtr->pageHeaderByteOffset = (p64)pageHeaderPtr - (p64)contextPtr->heap;
    pageInfoPtr->status = MEMORY_PAGE_STATUS_UNLOCKED;

    ((struct memory_page_key *)outPageKeyPtr)->pageId = pageId;
    ((struct memory_page_key *)outPageKeyPtr)->contextKey.contextId = contextPtr->id;
    ((struct memory_page_key *)outPageKeyPtr)->contextKey.isDebug = contextPtr->isDebug;

    return MEMORY_OK;
}

memory_error_code
memory_free_page(const struct memory_page_key *pageKeyPtr)
{
    if (!pageKeyPtr)
    {
        fprintf(stderr, "memory_free_page(%d): 'pageKeyPtr' is NULL. "
                "Cannot free a NULL page.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_ARGUMENT;
    }
    
    if (pageKeyPtr->pageId == MEMORY_SHORT_ID_NULL)
    {
        fprintf(stderr, "memory_free_page(%d): pageId in key is NULL ID. "
                "Cannot free a NULL page.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_ID;
    }
    
    if (pageKeyPtr->contextKey.contextId == MEMORY_SHORT_ID_NULL)
    {
        fprintf(stderr, "memory_free_page(%d): contextId in key is NULL ID. "
                "Cannot free a page without reference to its context.\n", __LINE__);
        
        return MEMORY_ERROR_NULL_ID;
    }

    struct memory_context *contextPtr;
    {
        memory_error_code resultCode;

        if ((resultCode = _memory_get_context((struct memory_context_key *)&pageKeyPtr->contextKey, 
            &contextPtr)) != MEMORY_OK)
        {
            return resultCode;
        }
    }

    struct memory_page_info *pageInfoPtr;
    {
        u16 infoIndex = pageKeyPtr->pageId - 1;

        if (infoIndex < contextPtr->pagesRegionInfoCount)
        {
            pageInfoPtr = &contextPtr->pagesRegionInfoArr[infoIndex];
        }
        else 
        {
            fprintf(stderr, "memory_free_page(%d): 'pageId' produced pageInfo index out of range. "
                    "Cannot free a non-existent page.\n", __LINE__);

            return MEMORY_ERROR_INDEX_OUT_OF_RANGE;
        }
    }

    if (pageInfoPtr->status == MEMORY_PAGE_STATUS_FREED)
    {
        fprintf(stderr, "memory_free_page(%d): Page is already freed.\n", __LINE__);

        return MEMORY_ERROR_PAGE_ALREADY_FREE;
    }

    pageInfoPtr->status = MEMORY_PAGE_STATUS_FREED;

    struct memory_page_header *pageHeaderPtr = (struct memory_page_header *)(contextPtr->heap + pageInfoPtr->pageHeaderByteOffset);

    if (contextPtr->pagesRegionActiveCount > 1)
    {
        contextPtr->activePages->prev->next = contextPtr->activePages->next;
        contextPtr->activePages->next->prev = contextPtr->activePages->prev;

        if (contextPtr->activePages == pageHeaderPtr)
        {
            contextPtr->activePages = contextPtr->activePages->next;
        }
    }
    else 
    {
        contextPtr->activePages = NULL;
    }

    --contextPtr->pagesRegionActiveCount;

    if (contextPtr->pagesRegionFreeCount > 0)
    {
        contextPtr->freePages->prev->next = pageHeaderPtr;
        pageHeaderPtr->prev = contextPtr->freePages->prev;
        contextPtr->freePages->prev = pageHeaderPtr;
        pageHeaderPtr->next = contextPtr->freePages;
    }
    else 
    {
        pageHeaderPtr->prev = pageHeaderPtr->next = pageHeaderPtr;
    }

    contextPtr->freePages = pageHeaderPtr;
    ++contextPtr->pagesRegionFreeCount;

    return MEMORY_OK;
}

memory_error_code
memory_realloc_page(const struct memory_page_key *pageKeyPtr, u64 byteSize)
{
    return MEMORY_ERROR_NOT_IMPLEMENTED;
}

memory_error_code
memory_get_page_key_is_ok(const struct memory_page_key *pageKeyPtr)
{
    return _memory_get_page_key_is_ok(pageKeyPtr);
}