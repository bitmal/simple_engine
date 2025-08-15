#include "unique_list.h"
#include "basic_list.h"
#include "memory.h"
#include "types.h"
#include "utils.h"

struct unique_list
{
    const struct memory_allocation_key basicListKey;
};

b32
_unique_list_is_byte_range_safe(const struct memory_allocation_key *listKeyPtr, 
    p64 startByteOffset, u64 byteSize)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        const struct memory_allocation_key databaseKey;
        p64 listDataByteOffset;
        u64 listDataByteSize;

        basic_list_get_database(&listPtr->basicListKey, &databaseKey, &listDataByteOffset, 
            &listDataByteSize);

        p64 startOffset = listDataByteOffset + startByteOffset;
        p64 endOffset = startOffset + byteSize;

        if (startOffset < listDataByteOffset)
        {
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }

        if (startOffset >= (listDataByteOffset + listDataByteSize))
        {
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }

        if (endOffset > (listDataByteOffset + listDataByteSize))
        {
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }
    }

    const struct memory_allocation_key headNodeKey;

    if (!(basic_list_get_next_node(&listPtr->basicListKey, NULL, &headNodeKey)))
    {
        memory_unmap_alloc((void **)&listPtr);

        return B32_TRUE;
    }

    const struct memory_allocation_key currentNodeKey;

    memcpy((void *)&currentNodeKey, &headNodeKey, sizeof(struct memory_allocation_key));

    do 
    {
        p64 currentByteOffset;
        u64 currentByteSize;

        basic_list_get_data_info(&listPtr->basicListKey, &currentNodeKey, 
        &currentByteOffset, &currentByteSize);

        if (((startByteOffset >= currentByteOffset) && (startByteOffset < (currentByteOffset + currentByteSize))) ||
            (((startByteOffset + byteSize) >= currentByteOffset) && ((startByteOffset + byteSize) < 
                (currentByteOffset + currentByteSize))))
        {
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }
    } while (!(MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)));

    memory_unmap_alloc((void **)&listPtr);

    return B32_TRUE;
}

b32
unique_list_create(const struct memory_page_key *pageKeyPtr, 
    p64 dataByteOffset, u64 dataByteSize, 
    const struct memory_allocation_key *databaseKeyPtr, 
    const struct memory_allocation_key *outListKeyPtr)
{
    if ((MEMORY_IS_PAGE_NULL(pageKeyPtr)))
    {
        return B32_FALSE;
    }
    if ((MEMORY_IS_ALLOCATION_NULL(databaseKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!outListKeyPtr)
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key uniqueListKey;
    struct unique_list *uniqueListPtr;
    {
        memory_error_code resultCode = memory_alloc(pageKeyPtr, sizeof(struct unique_list), NULL,
        &uniqueListKey);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&uniqueListKey, (void **)&uniqueListPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_free(&uniqueListKey);

            return B32_FALSE;
        }
    }

    const struct memory_allocation_key basicListKey;

    if (!(basic_list_create(pageKeyPtr, dataByteOffset, dataByteSize, 
        databaseKeyPtr, &basicListKey)))
    {
        memory_unmap_alloc((void **)&uniqueListPtr);
        memory_free(&uniqueListKey);

        return B32_FALSE;
    }

    memcpy((void *)&uniqueListPtr->basicListKey, &basicListKey, sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&uniqueListPtr);

    memcpy((void *)outListKeyPtr, &uniqueListKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
unique_list_get_node_id(const struct memory_allocation_key *nodeKeyPtr, 
    basic_list_id *outIdPtr)
{
    return basic_list_get_node_id(nodeKeyPtr, outIdPtr);
}

b32
unique_list_get_node_by_id(const struct memory_allocation_key *listKeyPtr, 
    basic_list_id nodeId, 
    const struct memory_allocation_key *outNodekeyPtr)
{
    return basic_list_get_node_by_id(listKeyPtr, nodeId, outNodekeyPtr);
}

b32
unique_list_get_is_node_active(const struct memory_allocation_key *nodeKeyPtr)
{
    return basic_list_get_is_node_active(nodeKeyPtr);
}

b32
unique_list_move_node(const struct memory_allocation_key *lhsListKeyPtr, 
    const struct memory_allocation_key *rhsListKeyPtr,
    const struct memory_allocation_key *nodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(lhsListKeyPtr)))
    {
        return B32_FALSE;
    }

    if ((MEMORY_IS_ALLOCATION_NULL(rhsListKeyPtr)))
    {
        return B32_FALSE;
    }
    
    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    // TODO:
    //if (!(_unique_list_is_byte_range_safe(rhsListKeyPtr, p64 startByteOffset, u64 byteSize)))

    struct unique_list *lhsListPtr, *rhsListPtr;
    {
        memory_error_code resultCode = memory_map_alloc(lhsListKeyPtr, (void **)&lhsListPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
        
        resultCode = memory_map_alloc(rhsListKeyPtr, (void **)&rhsListPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    if (!(basic_list_move_node(&lhsListPtr->basicListKey, &rhsListPtr->basicListKey, nodeKeyPtr)))
    {
        memory_unmap_alloc((void **)&lhsListPtr);
        memory_unmap_alloc((void **)&rhsListPtr);

        return B32_FALSE;
    }

    memory_unmap_alloc((void **)&lhsListPtr);
    memory_unmap_alloc((void **)&rhsListPtr);

    return B32_TRUE;
}

b32
unique_list_move_free_node(const struct memory_allocation_key *lhsListKeyPtr,
    const struct memory_allocation_key *rhsListKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(lhsListKeyPtr)))
    {
        if (outNodeKeyPtr)
        {
            memory_get_null_allocation_key(outNodeKeyPtr);
        }

        return B32_FALSE;
    }
    
    if ((MEMORY_IS_ALLOCATION_NULL(rhsListKeyPtr)))
    {
        if (outNodeKeyPtr)
        {
            memory_get_null_allocation_key(outNodeKeyPtr);
        }

        return B32_FALSE;
    }

    if (!outNodeKeyPtr)
    {
        return B32_FALSE;
    }

    struct unique_list *lhsListPtr, *rhsListPtr;
    {
        memory_error_code resultCode = memory_map_alloc(lhsListKeyPtr, (void **)&lhsListPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_get_null_allocation_key(outNodeKeyPtr);

            return B32_FALSE;
        }
        
        resultCode = memory_map_alloc(rhsListKeyPtr, (void **)&rhsListPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&lhsListPtr);
            memory_get_null_allocation_key(outNodeKeyPtr);

            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_move_free_node(&lhsListPtr->basicListKey, &rhsListPtr->basicListKey, 
        dataByteOffset, dataByteSize, outNodeKeyPtr);

    memory_unmap_alloc((void **)&rhsListPtr);
    memory_unmap_alloc((void **)&lhsListPtr);

    return isResult;
}

b32
unique_list_get_active_count(const struct memory_allocation_key *listKeyPtr, 
    u64 *outActiveCount)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!outActiveCount)
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_get_active_count(&listPtr->basicListKey, outActiveCount);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_get_next_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!outNodeKeyPtr)
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_get_next_node(&listPtr->basicListKey, nodeKeyPtr, outNodeKeyPtr);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_get_prev_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!outNodeKeyPtr)
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_get_prev_node(&listPtr->basicListKey, nodeKeyPtr, outNodeKeyPtr);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_map_data(const struct memory_allocation_key *nodeKeyPtr, void **outDataPtr)
{
    return basic_list_map_data(nodeKeyPtr, outDataPtr);
}

b32
unique_list_unmap_data(const struct memory_allocation_key *listKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_unmap_data(&listPtr->basicListKey);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_set_data_info(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr,
    p64 dataByteOffset, u64 dataByteSize)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!(basic_list_get_is_node_active(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_set_data_info(&listPtr->basicListKey, nodeKeyPtr, dataByteOffset, dataByteSize);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_get_data_info(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr,
    p64 *dataByteOffset, u64 *dataByteSize)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!(basic_list_get_is_node_active(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_get_data_info(&listPtr->basicListKey, nodeKeyPtr, dataByteOffset, dataByteSize);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_get_database(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *outDatabaseKeyPtr,
    p64 *outDataByteOffset, u64 *outDataByteSize)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_get_database(&listPtr->basicListKey, outDatabaseKeyPtr, outDataByteOffset, 
        outDataByteSize);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_reset_data(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *databaseKeyPtr, p64 dataByteOffset,
    u64 dataByteSize)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_reset_data(&listPtr->basicListKey, databaseKeyPtr, dataByteOffset, dataByteSize);
    
    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_append(const struct memory_allocation_key *listKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (!(_unique_list_is_byte_range_safe(listKeyPtr, dataByteOffset, dataByteSize)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_append(&listPtr->basicListKey, dataByteOffset, dataByteSize, outNodeKeyPtr);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_insert_front(const struct memory_allocation_key *listKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (!(_unique_list_is_byte_range_safe(listKeyPtr, dataByteOffset, dataByteSize)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_insert_front(&listPtr->basicListKey, dataByteOffset, dataByteSize, outNodeKeyPtr);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_append_after(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (!(_unique_list_is_byte_range_safe(listKeyPtr, dataByteOffset, dataByteSize)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_append_after(&listPtr->basicListKey, lhsNodeKeyPtr, dataByteOffset, 
        dataByteSize, outNodeKeyPtr);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_insert_before(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (!(_unique_list_is_byte_range_safe(listKeyPtr, dataByteOffset, dataByteSize)))
    {
        return B32_FALSE;
    }

    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_insert_before(&listPtr->basicListKey, lhsNodeKeyPtr, dataByteOffset, 
        dataByteSize, outNodeKeyPtr);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_free_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr)
{
    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_free_node(&listPtr->basicListKey, nodeKeyPtr);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_clear_nodes(const struct memory_allocation_key *listKeyPtr)
{
    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_clear_nodes(&listPtr->basicListKey);

    memory_unmap_alloc((void **)&listPtr);

    return isResult;
}

b32
unique_list_destroy(const struct memory_allocation_key *listKeyPtr)
{
    struct unique_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    basic_list_destroy(&listPtr->basicListKey);

    memory_unmap_alloc((void **)&listPtr);

    memory_free(listKeyPtr);

    return B32_TRUE;
}