#include "basic_list.h"
#include "memory.h"
#include "basic_list.h"
#include "types.h"
#include "utils.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct basic_list_node
{
    basic_list_id id;
    const struct memory_allocation_key listKey;
    const struct memory_allocation_key dataKey;
    p64 dataByteOffset;
    u64 dataByteSize;
    const struct memory_allocation_key prevNodeKey;
    const struct memory_allocation_key nextNodeKey;
    b32 isActive;
};

struct basic_list
{
    const struct memory_page_key memoryPageKey;
    u64 activeListNodeCount;
    const struct memory_allocation_key activeNodeKeyList;
    u64 freeListNodeCount;
    const struct memory_allocation_key freeNodeKeyList;
};

b32
_basic_list_generate_id(basic_list_id *outIdPtr)
{
    if (!outIdPtr)
    {
        return B32_FALSE;
    }

    return utils_generate_random_u64(outIdPtr);
}

b32
_basic_list_assign_id(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *nodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }
    
    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    struct basic_list *listPtr;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(nodeKeyPtr, (void **)&nodePtr);
        
        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }
    }

    basic_list_id id;
    {
        if (listPtr->activeListNodeCount > 0)
        {
            while (B32_TRUE)
            {
                if (!(_basic_list_generate_id(&id)))
                {
                    memory_unmap_alloc((void **)&nodePtr);
                    memory_unmap_alloc((void **)&listPtr);

                    return B32_FALSE;
                }

                if (id == BASIC_LIST_NULL_ID)
                {
                    continue;
                }

                const struct memory_allocation_key currentNodeKey;

                memcpy((void *)&currentNodeKey, &listPtr->activeNodeKeyList, 
                sizeof(struct memory_allocation_key));

                do 
                {
                    struct basic_list_node *currentNodePtr;

                    memory_error_code resultCode = memory_map_alloc(&currentNodeKey, 
                    (void **)&currentNodePtr);

                    if (resultCode != MEMORY_OK)
                    {
                        memory_unmap_alloc((void **)&nodePtr);
                        memory_unmap_alloc((void **)&listPtr);

                        return B32_FALSE;
                    }

                    if (currentNodePtr->id != id)
                    {
                        if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&currentNodePtr->nextNodeKey, &listPtr->activeNodeKeyList)))
                        {
                            memcpy((void *)&currentNodeKey, &currentNodePtr->nextNodeKey, 
                            sizeof(struct memory_allocation_key));
                        }
                        else 
                        {
                            memory_unmap_alloc((void **)&currentNodePtr);
                            memory_get_null_allocation_key(&currentNodeKey);

                            break;
                        }
                    }
                    else
                    {
                        memory_unmap_alloc((void **)&currentNodePtr);

                        break;
                    }
                    
                    memory_unmap_alloc((void **)&currentNodePtr);
                } while (!(MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)));

                if ((MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)))
                {
                    break;
                }
            }
        }
        else 
        {
            if (!(_basic_list_generate_id(&id)))
            {
                memory_unmap_alloc((void **)&nodePtr);
                memory_unmap_alloc((void **)&listPtr);

                return B32_FALSE;
            }
        }
    }

    nodePtr->id = id;

    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

    return B32_TRUE;
}

static b32
_basic_list_get_next_free_node(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *outFreeNodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!outFreeNodeKeyPtr)
    {
        return B32_FALSE;
    }

    struct basic_list *listPtr;
    const struct memory_allocation_key nodeKey;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, 
        (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            goto result_false;
        }

        if (listPtr->freeListNodeCount > 0)
        {
            struct basic_list_node *nodePtr;

            resultCode = memory_map_alloc(&listPtr->freeNodeKeyList, 
            (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                goto result_false;
            }

            if (listPtr->freeListNodeCount > 1)
            {
                struct basic_list_node *prevNodePtr;

                resultCode = memory_map_alloc(&nodePtr->prevNodeKey, 
                (void **)&prevNodePtr);

                if (resultCode != MEMORY_OK)
                {
                    memory_unmap_alloc((void **)nodePtr);

                    goto result_false;
                }
                
                struct basic_list_node *nextNodePtr;

                resultCode = memory_map_alloc(&nodePtr->nextNodeKey, 
                (void **)&nextNodePtr);

                if (resultCode != MEMORY_OK)
                {
                    memory_unmap_alloc((void **)prevNodePtr);
                    memory_unmap_alloc((void **)nodePtr);

                    goto result_false;
                }
                
                memcpy((void *)&nodeKey, &listPtr->freeNodeKeyList, sizeof(struct memory_allocation_key));
                memcpy((void *)&listPtr->freeNodeKeyList, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));

                memcpy((void *)&prevNodePtr->nextNodeKey, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));
                memcpy((void *)&nextNodePtr->prevNodeKey, &nodePtr->prevNodeKey, sizeof(struct memory_allocation_key));

                memory_unmap_alloc((void **)prevNodePtr);
                memory_unmap_alloc((void **)nextNodePtr);
            }
            else 
            {
                memcpy((void *)&nodeKey, &listPtr->freeNodeKeyList, sizeof(struct memory_allocation_key));
            }

            memory_get_null_allocation_key(&nodePtr->prevNodeKey);
            memory_get_null_allocation_key(&nodePtr->nextNodeKey);

            memory_unmap_alloc((void **)nodePtr);
            
            --listPtr->freeListNodeCount;
        }
        else 
        {
            struct basic_list_node *nodePtr;

            resultCode = memory_alloc(&listPtr->memoryPageKey, sizeof(struct basic_list_node), 
            NULL, &nodeKey);

            if (resultCode != MEMORY_OK)
            {
                goto result_false;
            }

            memory_unmap_alloc((void **)&listPtr);

            if (!(_basic_list_assign_id(listKeyPtr, &nodeKey)))
            {
                memory_free(&nodeKey);

                return B32_FALSE;
            }
            
            resultCode = memory_map_alloc(&nodeKey, (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_free(&nodeKey);

                goto result_false;
            }

            nodePtr->isActive = B32_FALSE;

            memcpy((void *)&nodePtr->listKey, listKeyPtr, sizeof(struct memory_allocation_key));
            
            memory_get_null_allocation_key(&nodePtr->prevNodeKey);
            memory_get_null_allocation_key(&nodePtr->nextNodeKey);

            memory_unmap_alloc((void **)&nodePtr);
        }
    }

    goto result_true;

result_false:
    memory_unmap_alloc((void **)&listPtr);
    memory_get_null_allocation_key(outFreeNodeKeyPtr);
    
    return B32_FALSE;

result_true:
    memory_unmap_alloc((void **)&listPtr);
    memcpy((void *)outFreeNodeKeyPtr, &nodeKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
_basic_list_free_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    struct basic_list *listPtr;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(nodeKeyPtr, (void **)&nodePtr);
        
        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }
        
        if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(listKeyPtr, &nodePtr->listKey)))
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }
    }

    if (nodePtr->isActive)
    {
        if (listPtr->activeListNodeCount > 1)
        {
            struct basic_list_node *nextNodePtr;

            memory_error_code resultCode = memory_map_alloc(&nodePtr->nextNodeKey, (void **)&nextNodePtr);

            if (resultCode != MEMORY_OK)
            {
                goto result_false;
            }
            
            struct basic_list_node *prevNodePtr;

            resultCode = memory_map_alloc(&nodePtr->prevNodeKey, (void **)&prevNodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&nextNodePtr);

                goto result_false;
            }

            memcpy((void *)&nextNodePtr->prevNodeKey, &nodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&prevNodePtr->nextNodeKey, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));

            if ((MEMORY_IS_ALLOCATION_KEY_EQUAL(nodeKeyPtr, &listPtr->activeNodeKeyList)))
            {
                memcpy((void *)&listPtr->activeNodeKeyList, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));
            }

            memory_unmap_alloc((void **)&prevNodePtr);
            memory_unmap_alloc((void **)&nextNodePtr);
        }

        --listPtr->activeListNodeCount;
    }

    if (listPtr->freeListNodeCount > 0)
    {
        struct basic_list_node *nextNodePtr;

        memory_error_code resultCode = memory_map_alloc(&listPtr->freeNodeKeyList, (void **)&nextNodePtr);

        if (resultCode != MEMORY_OK)
        {
            goto result_false;
        }
        
        struct basic_list_node *tailNodePtr;

        resultCode = memory_map_alloc(&nextNodePtr->prevNodeKey, (void **)&tailNodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&nextNodePtr);

            goto result_false;
        }

        memcpy((void *)&nodePtr->prevNodeKey, &nextNodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->nextNodeKey, &listPtr->freeNodeKeyList, sizeof(struct memory_allocation_key));
        
        memcpy((void *)&nextNodePtr->prevNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
        memcpy((void *)&tailNodePtr->nextNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));

        memory_unmap_alloc((void **)tailNodePtr);
        memory_unmap_alloc((void **)nextNodePtr);
    }
    else 
    {
        memcpy((void *)&nodePtr->prevNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->nextNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
    }

    memcpy((void *)&nodePtr->listKey, listKeyPtr, sizeof(struct memory_allocation_key));
    memcpy((void *)&listPtr->freeNodeKeyList, nodeKeyPtr, sizeof(struct memory_allocation_key));

    nodePtr->isActive = B32_FALSE;

    ++listPtr->freeListNodeCount;

    goto result_true;

result_false:
    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

    return B32_FALSE;

result_true:
    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

    return B32_TRUE;
}

b32
_basic_list_activate_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr,
    const struct memory_allocation_key *dataKeyPtr,
    p64 dataByteOffset, u64 dataByteSize)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    struct basic_list *listPtr;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(nodeKeyPtr, (void **)&nodePtr);
        
        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }
    }

    if (nodePtr->isActive)
    {
        goto result_false;
    }

    if (listPtr->activeListNodeCount > 0)
    {
        struct basic_list_node *nextNodePtr;

        memory_error_code resultCode = memory_map_alloc(&listPtr->activeNodeKeyList, 
        (void **)&nextNodePtr);

        if (resultCode != MEMORY_OK)
        {
            goto result_false;
        }
        
        struct basic_list_node *tailNodePtr;

        resultCode = memory_map_alloc(&nextNodePtr->prevNodeKey, (void **)&tailNodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&nextNodePtr);

            goto result_false;
        }

        memcpy((void *)&nodePtr->prevNodeKey, &nextNodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->nextNodeKey, &listPtr->activeNodeKeyList, sizeof(struct memory_allocation_key));
        
        memcpy((void *)&nextNodePtr->prevNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
        memcpy((void *)&tailNodePtr->nextNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));

        memory_unmap_alloc((void **)tailNodePtr);
        memory_unmap_alloc((void **)nextNodePtr);
    }
    else 
    {
        memcpy((void *)&nodePtr->prevNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->nextNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
    }

    memcpy((void *)&nodePtr->listKey, listKeyPtr, sizeof(struct memory_allocation_key));
    memcpy((void *)&listPtr->activeNodeKeyList, nodeKeyPtr, sizeof(struct memory_allocation_key));

    nodePtr->isActive = B32_TRUE;

    nodePtr->dataByteOffset = dataByteOffset;
    nodePtr->dataByteSize = dataByteSize;

    memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));

    ++listPtr->activeListNodeCount;

    goto result_true;

result_false:
    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

    return B32_FALSE;

result_true:
    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

    return B32_TRUE;
}

b32
basic_list_create(const struct memory_page_key *memoryPageKeyPtr, 
    const struct memory_allocation_key *outListKeyPtr)
{
    if ((MEMORY_IS_PAGE_NULL(memoryPageKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!outListKeyPtr)
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key listKey;
    {
       memory_error_code resultCode;

        resultCode = memory_alloc(memoryPageKeyPtr, 
            sizeof(struct basic_list), NULL, &listKey);

        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): "
                "Could not allocate new list. Aborting.", __func__, __LINE__);

            return B32_FALSE;
        }
    }

    struct basic_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(&listKey, 
        (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_free(&listKey);

            return B32_FALSE;
        }
    }

    memory_set_alloc_offset_width(&listKey, 0, 
    sizeof(struct basic_list), '\0');

    memcpy((void *)&listPtr->memoryPageKey, (void *)memoryPageKeyPtr, 
        sizeof(struct memory_page_key));

    memory_unmap_alloc((void **)&listPtr);
    
    memcpy((void *)outListKeyPtr, (void *)&listKey, 
        sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
basic_list_get_node_id(const struct memory_allocation_key *nodeKeyPtr, 
    basic_list_id *outIdPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!outIdPtr)
    {
        return B32_FALSE;
    }

    struct basic_list_node *nodePtr;

    memory_error_code resultCode = memory_map_alloc(nodeKeyPtr, (void **)&nodePtr);

    if (resultCode != MEMORY_OK)
    {
        return B32_FALSE;
    }

    *outIdPtr = nodePtr->id;

    memory_unmap_alloc((void **)&nodePtr);

    return B32_TRUE;
}

b32
basic_list_get_node_by_id(const struct memory_allocation_key *listKeyPtr, 
    basic_list_id nodeId, 
    const struct memory_allocation_key *outNodekeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    if (nodeId == BASIC_LIST_NULL_ID)
    {
        return B32_FALSE;
    }

    if (!outNodekeyPtr)
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key headNodeKey;

    if (!(basic_list_get_next_node(listKeyPtr, NULL, &headNodeKey)))
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key currentNodeKey;

    memcpy((void *)&currentNodeKey, &headNodeKey, sizeof(struct memory_allocation_key));

    do 
    {
        struct basic_list_node *nodePtr;

        memory_error_code resultCode = memory_map_alloc(&currentNodeKey, (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        if (nodePtr->id == nodeId)
        {
            memcpy((void *)outNodekeyPtr, &currentNodeKey, sizeof(const struct memory_allocation_key));
            memory_unmap_alloc((void *)&nodePtr);

            return B32_TRUE;
        }

        if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&headNodeKey, &nodePtr->nextNodeKey)))
        {
            memcpy((void *)&currentNodeKey, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));
        }
        else
        {
            memory_get_null_allocation_key(&currentNodeKey);
        }

        memory_unmap_alloc((void **)&nodePtr);
    } while (!(MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)));

    return B32_FALSE;
}
b32
basic_list_get_is_node_active(const struct memory_allocation_key *nodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(nodeKeyPtr, (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isActive = nodePtr->isActive;

    memory_unmap_alloc((void **)&nodePtr);

    return isActive;
}

b32
basic_list_get_active_count(const struct memory_allocation_key *listKeyPtr, 
    u64 *outActiveCount)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        // TODO:

        return B32_FALSE;
    }

    if (!outActiveCount)
    {
        // TODO:

        return B32_FALSE;
    }

    struct basic_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }
    }

    *outActiveCount = listPtr->activeListNodeCount;

    return B32_TRUE;
}

b32
basic_list_get_next_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (!outNodeKeyPtr)
    {
        // TODO:

        return B32_FALSE;
    }

    struct basic_list_node *nodePtr;

    if (!MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr))
    {
        memory_error_code resultCode = memory_map_alloc(nodeKeyPtr, 
        (void **)nodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_set_alloc_offset_width(outNodeKeyPtr, 0, 
            sizeof(struct memory_allocation_key), '\0');

            // TODO:

            return B32_FALSE;
        }
    }
    else 
    {
        nodePtr = NULL;
    }

    struct basic_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, 
        (void **)listPtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:
            if (nodePtr)
            {
                memory_unmap_alloc((void **)&nodePtr);
                memory_set_alloc_offset_width(outNodeKeyPtr, 0, 
                sizeof(struct memory_allocation_key), '\0');
            }

            return B32_FALSE;
        }
    }

    if (nodePtr)
    {
        if (nodePtr->isActive)
        {
            memcpy((void *)outNodeKeyPtr, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));
            memory_unmap_alloc((void **)&nodePtr);
        }
        else
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&listPtr);
            
            memory_set_alloc_offset_width(outNodeKeyPtr, 0, 
            sizeof(struct memory_allocation_key), '\0');

            // TODO:

            return B32_FALSE;
        }
    }
    else 
    {
        if (listPtr->activeListNodeCount > 0)
        {
            memcpy((void *)outNodeKeyPtr, &listPtr->activeNodeKeyList, sizeof(struct memory_allocation_key));
        }
        else 
        {
            memory_set_alloc_offset_width(outNodeKeyPtr, 0, 
            sizeof(struct memory_allocation_key), '\0');

            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }
    }

    memory_unmap_alloc((void **)&listKeyPtr);

    return B32_TRUE;
}

b32
basic_list_get_prev_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (!outNodeKeyPtr)
    {
        // TODO:

        return B32_FALSE;
    }

    struct basic_list_node *nodePtr;

    if (!MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr))
    {
        memory_error_code resultCode = memory_map_alloc(nodeKeyPtr, 
        (void **)nodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_set_alloc_offset_width(outNodeKeyPtr, 0, 
            sizeof(struct memory_allocation_key), '\0');

            // TODO:

            return B32_FALSE;
        }
    }
    else 
    {
        nodePtr = NULL;
    }

    struct basic_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, 
        (void **)listPtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:
            if (nodePtr)
            {
                memory_unmap_alloc((void **)&nodePtr);
            }

            memory_set_alloc_offset_width(outNodeKeyPtr, 0, 
            sizeof(struct memory_allocation_key), '\0');

            return B32_FALSE;
        }
    }

    if (nodePtr)
    {
        if (nodePtr->isActive)
        {
            memcpy((void *)outNodeKeyPtr, &nodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
            memory_unmap_alloc((void **)&nodePtr);
        }
        else
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&listPtr);
            memory_set_alloc_offset_width(outNodeKeyPtr, 0, 
            sizeof(struct memory_allocation_key), '\0');

            // TODO:

            return B32_FALSE;
        }
    }
    else 
    {
        if (listPtr->activeListNodeCount > 0)
        {
            struct basic_list_node *headNodePtr;

            memory_error_code resultCode = memory_map_alloc(&listPtr->activeNodeKeyList, 
            (void **)&headNodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&headNodePtr);
                memory_unmap_alloc((void **)&listPtr);
                memory_set_alloc_offset_width(outNodeKeyPtr, 0, 
                sizeof(struct memory_allocation_key), '\0');

                // TODO:

                return B32_FALSE;
            }

            memcpy((void *)outNodeKeyPtr, &headNodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
            memory_unmap_alloc((void **)&headNodePtr);
        }
        else 
        {
            memory_get_null_allocation_key(outNodeKeyPtr);
            memory_unmap_alloc((void **)&listPtr);

            // TODO:

            return B32_FALSE;
        }
    }

    memory_unmap_alloc((void **)&listKeyPtr);

    return B32_TRUE;
}

b32
basic_list_map_data(const struct memory_allocation_key *nodeKeyPtr, void **outDataPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        if (outDataPtr)
        {
            *outDataPtr = NULL;
        }

        return B32_FALSE;
    }

    if (!outDataPtr)
    {
        return B32_FALSE;
    }

    struct basic_list *listPtr;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(nodeKeyPtr, (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            *outDataPtr = NULL;

            return B32_FALSE;
        }
        
        resultCode = memory_map_alloc(&nodePtr->listKey, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            *outDataPtr = NULL;

            memory_unmap_alloc((void **)&nodePtr);

            return B32_FALSE;
        }
    }

    u8 *resultPtr;
    
    memory_error_code resultCode = memory_map_alloc(&nodePtr->dataKey, 
    (void **)&resultPtr);

    if (resultCode != MEMORY_OK)
    {
        memory_unmap_alloc((void **)&listPtr);
        memory_unmap_alloc((void **)&nodePtr);

        return B32_FALSE;
    }

    resultPtr += nodePtr->dataByteOffset;
    *outDataPtr = resultPtr;

    memory_unmap_alloc((void **)&listPtr);
    memory_unmap_alloc((void **)&nodePtr);

    return B32_TRUE;
}

b32
basic_list_unmap_data(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr, void **dataPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }
    
    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!dataPtr)
    {
        return B32_FALSE;
    }

    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(nodeKeyPtr, 
        (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    u8 *resultPtr = *dataPtr;
    resultPtr -= nodePtr->dataByteOffset;

    memory_error_code resultCode = memory_unmap_alloc((void **)&resultPtr);

    if (resultCode != MEMORY_OK)
    {
        memory_unmap_alloc((void **)&nodePtr);

        return B32_FALSE;
    }

    memory_unmap_alloc((void **)&nodePtr);

    return B32_TRUE;
}

b32
basic_list_set_data_info(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr,
    const struct memory_allocation_key *dataKeyPtr,
    p64 *dataByteOffset, u64 *dataByteSize)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }
    
    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    struct basic_list *listPtr;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(nodeKeyPtr, (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }
    }

    if (dataByteOffset)
    {
        nodePtr->dataByteOffset = *dataByteOffset;
    }

    if (dataByteSize)
    {
        nodePtr->dataByteSize = *dataByteSize;
    }

    if (!(MEMORY_IS_ALLOCATION_NULL(dataKeyPtr)))
    {
        memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));
    }
    else if (dataKeyPtr)
    {
        memory_get_null_allocation_key(&nodePtr->dataKey);
    }

goto result_true;

result_false:
    memory_unmap_alloc((void **)nodePtr);
    memory_unmap_alloc((void **)listPtr);

return B32_FALSE;

result_true:
    memory_unmap_alloc((void **)nodePtr);
    memory_unmap_alloc((void **)listPtr);

    return B32_TRUE;
}

b32
basic_list_get_data_info(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr,
    const struct memory_allocation_key *outDataKeyPtr,
    p64 *dataByteOffset, u64 *dataByteSize)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }
    
    if ((MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        return B32_FALSE;
    }

    struct basic_list *listPtr;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(nodeKeyPtr, (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&listPtr);

            return B32_FALSE;
        }

        if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(listKeyPtr, &nodePtr->listKey)))
        {
            goto result_false;
        }

        if (!nodePtr->isActive)
        {
            goto result_false;
        }
    }

    if (dataByteSize)
    {
        *dataByteSize = nodePtr->dataByteSize;
    }
    
    if (dataByteOffset)
    {
        *dataByteOffset = nodePtr->dataByteOffset;
    }

    if (outDataKeyPtr)
    {
        memcpy((void *)outDataKeyPtr, &nodePtr->dataKey, sizeof(struct memory_allocation_key));
    }

goto result_true;

result_false:
    memory_unmap_alloc((void **)nodePtr);
    memory_unmap_alloc((void **)listPtr);

return B32_FALSE;

result_true:
    memory_unmap_alloc((void **)nodePtr);
    memory_unmap_alloc((void **)listPtr);

    return B32_TRUE;
}

b32
basic_list_append(const struct memory_allocation_key *listKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'listKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    if (!outNodeKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'outNodeKeyPtr' argument cannot be NULL. Despite this, the argument/dereferenced value does not matter, "
            "as it is an out parameter. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    struct basic_list *listPtr;
    const struct memory_allocation_key nodeKey;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = MEMORY_OK;

        if ((_basic_list_get_next_free_node(listKeyPtr, &nodeKey)))
        {
            resultCode = memory_map_alloc(&nodeKey, (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&nodePtr);

                return B32_FALSE;
            }
        }
        else
        {
            memory_unmap_alloc((void **)&nodePtr);

            return B32_FALSE;
        }
    
        resultCode = memory_map_alloc(listKeyPtr, 
        (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): "
                "Could not memory map list. Aborting.", __func__, __LINE__);

            return B32_FALSE;
        }

        if (listPtr->activeListNodeCount > 0)
        {
            struct basic_list_node *headNodePtr;

            resultCode = memory_map_alloc(&listPtr->activeNodeKeyList, (void **)&headNodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&nodePtr);
                memory_unmap_alloc((void **)&listPtr);

                return B32_FALSE;
            }
            
            struct basic_list_node *tailNodePtr;

            resultCode = memory_map_alloc(&headNodePtr->prevNodeKey, (void **)tailNodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&headNodePtr);
                memory_unmap_alloc((void **)&nodePtr);
                memory_unmap_alloc((void **)&listPtr);

                return B32_FALSE;
            }

            memcpy((void *)&nodePtr->prevNodeKey, &headNodePtr->prevNodeKey, 
            sizeof(struct memory_allocation_key));

            memcpy((void *)&nodePtr->nextNodeKey, &listPtr->activeNodeKeyList, 
            sizeof(struct memory_allocation_key));
            
            memcpy((void *)&headNodePtr->prevNodeKey, &nodeKey, 
            sizeof(struct memory_allocation_key));
            
            memcpy((void *)&tailNodePtr->nextNodeKey, &nodeKey, 
            sizeof(struct memory_allocation_key));
            
            memory_unmap_alloc((void **)&tailNodePtr);
            memory_unmap_alloc((void **)&headNodePtr);
        }
        else 
        {
            memcpy((void *)&nodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&listPtr->activeNodeKeyList, &nodeKey, sizeof(struct memory_allocation_key));
        }

        ++listPtr->activeListNodeCount;
    }

    nodePtr->isActive = B32_TRUE;
    nodePtr->dataByteOffset = dataByteOffset;
    nodePtr->dataByteSize = dataByteSize;

    memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);
    
    memcpy((void *)outNodeKeyPtr, &nodeKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
basic_list_insert_front(const struct memory_allocation_key *listKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (MEMORY_IS_ALLOCATION_NULL(listKeyPtr))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'listKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    if (!outNodeKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'outNodeKeyPtr' argument cannot be NULL. Despite this, the argument/dereferenced value does not matter, "
            "as it is an out parameter. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    const struct memory_allocation_key nodeKey;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = MEMORY_OK;

        if (!(_basic_list_get_next_free_node(listKeyPtr, &nodeKey)))
        {
            return B32_FALSE;
        }

        if (!(_basic_list_activate_node(listKeyPtr, &nodeKey, dataKeyPtr, 
            dataByteOffset, dataByteSize)))
        {
            memory_unmap_alloc((void **)&nodePtr);

            return B32_FALSE;
        }
    }

    memcpy((void *)outNodeKeyPtr, &nodeKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
basic_list_append_after(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'listKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }
    
    if ((MEMORY_IS_ALLOCATION_NULL(lhsNodeKeyPtr)))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'lhsNodeKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    if (!outNodeKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'outNodeKeyPtr' argument cannot be NULL. Despite this, the argument/dereferenced value does not matter, "
            "as it is an out parameter. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    const struct memory_allocation_key nodeKey;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode;

        if ((_basic_list_get_next_free_node(listKeyPtr, &nodeKey)))
        {
            resultCode = memory_map_alloc(&nodeKey, (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&nodePtr);

                return B32_FALSE;
            }
        }
        else
        {
            return B32_FALSE;
        }

        struct basic_list *listPtr;

        resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&nodePtr);
            _basic_list_free_node(listKeyPtr, &nodeKey);

            return B32_FALSE;
        }

        if (listPtr->activeListNodeCount > 0)
        {
            if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(listKeyPtr, lhsNodeKeyPtr)))
            {
                memory_unmap_alloc((void **)&listPtr);
                _basic_list_free_node(listKeyPtr, &nodeKey);

                return B32_FALSE;
            }

            struct basic_list_node *lhsNodePtr;

            resultCode = memory_map_alloc(lhsNodeKeyPtr, (void **)&lhsNodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&listPtr);
                _basic_list_free_node(listKeyPtr, &nodeKey);

                return B32_FALSE;
            }
            
            struct basic_list_node *nextNodePtr;

            resultCode = memory_map_alloc(&lhsNodePtr->nextNodeKey, (void **)&nextNodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&lhsNodePtr);
                memory_unmap_alloc((void **)&listPtr);
                _basic_list_free_node(listKeyPtr, &nodeKey);

                return B32_FALSE;
            }

            memcpy((void *)&nodePtr->prevNodeKey, lhsNodeKeyPtr, sizeof(struct memory_allocation_key));
            memcpy((void *)&nodePtr->nextNodeKey, &lhsNodePtr->nextNodeKey, sizeof(struct memory_allocation_key));
            
            memcpy((void *)&lhsNodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nextNodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));

            ++listPtr->activeListNodeCount;

            memory_unmap_alloc((void **)&nextNodePtr);
            memory_unmap_alloc((void **)&lhsNodePtr);
            memory_unmap_alloc((void **)&listPtr);
        }
        else 
        {
            memory_unmap_alloc((void **)&listPtr);
            _basic_list_free_node(listKeyPtr, &nodeKey);

            return B32_FALSE;
        }
    }

    nodePtr->dataByteOffset = dataByteOffset;
    nodePtr->dataByteSize = dataByteSize;
    nodePtr->isActive = B32_TRUE;

    if (!(MEMORY_IS_ALLOCATION_NULL(dataKeyPtr)))
    {
        memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));
    }
    else
    {
        memory_get_null_allocation_key(&nodePtr->dataKey);
    }

    memcpy((void *)&nodePtr->listKey, listKeyPtr, sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&nodePtr);
    
    memcpy((void *)outNodeKeyPtr, &nodeKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
basic_list_move_node(const struct memory_allocation_key *lhsListKeyPtr, 
    const struct memory_allocation_key *rhsListKeyPtr,
    const struct memory_allocation_key *nodeKeyPtr)
{
    if (!(MEMORY_IS_ALLOCATION_NULL(lhsListKeyPtr)))
    {
        // TODO:

        return B32_FALSE;
    }
    
    if (!(MEMORY_IS_ALLOCATION_NULL(rhsListKeyPtr)))
    {
        // TODO:

        return B32_FALSE;
    }
    
    if (!(MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr)))
    {
        // TODO:

        return B32_FALSE;
    }

    struct basic_list *lhsListPtr;
    struct basic_list *rhsListPtr;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(lhsListKeyPtr, (void **)lhsListPtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }
        
        resultCode = memory_map_alloc(rhsListKeyPtr, (void **)rhsListPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&lhsListPtr);

            // TODO:

            return B32_FALSE;
        }
        
        resultCode = memory_map_alloc(nodeKeyPtr, (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&lhsListPtr);
            memory_unmap_alloc((void **)&rhsListPtr);

            // TODO:

            return B32_FALSE;
        }

        if ((MEMORY_IS_ALLOCATION_KEY_EQUAL(lhsListKeyPtr, rhsListKeyPtr)))
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&lhsListPtr);
            memory_unmap_alloc((void **)&rhsListPtr);

            // TODO:

            return B32_FALSE;
        }

        if (!nodePtr->isActive)
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&lhsListPtr);
            memory_unmap_alloc((void **)&rhsListPtr);

            // TODO:

            return B32_FALSE;
        }
    }

    if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(lhsListKeyPtr, &nodePtr->listKey)))
    {
        memory_unmap_alloc((void **)&nodePtr);
        memory_unmap_alloc((void **)&lhsListPtr);
        memory_unmap_alloc((void **)&rhsListPtr);

        // TODO:

        return B32_FALSE;
    }

    if (lhsListPtr->activeListNodeCount > 1)
    {
        memory_error_code resultCode;

        struct basic_list_node *prevNodePtr, *nextNodePtr;

        resultCode = memory_map_alloc(&nodePtr->prevNodeKey, (void **)&prevNodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&lhsListPtr);
            memory_unmap_alloc((void **)&rhsListPtr);

            // TODO:

            return B32_FALSE;
        }
        
        resultCode = memory_map_alloc(&nodePtr->nextNodeKey, (void **)&nextNodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&prevNodePtr);
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&lhsListPtr);
            memory_unmap_alloc((void **)&rhsListPtr);

            // TODO:

            return B32_FALSE;
        }
        
        if ((MEMORY_IS_ALLOCATION_KEY_EQUAL(&lhsListPtr->activeNodeKeyList, nodeKeyPtr)))
        {
            memcpy((void *)&lhsListPtr->activeNodeKeyList, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));
        }

        memcpy((void *)&prevNodePtr->nextNodeKey, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));
        memcpy((void *)&nextNodePtr->prevNodeKey, &nodePtr->prevNodeKey, sizeof(struct memory_allocation_key));


        memory_unmap_alloc((void **)&prevNodePtr);
        memory_unmap_alloc((void **)&nextNodePtr);
    }

    --lhsListPtr->activeListNodeCount;

    if (rhsListPtr->activeListNodeCount > 0)
    {
        memory_error_code resultCode;

        struct basic_list_node *prevNodePtr, *nextNodePtr;
        
        resultCode = memory_map_alloc(&rhsListPtr->activeNodeKeyList, (void **)&nextNodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&lhsListPtr);
            memory_unmap_alloc((void **)&rhsListPtr);

            // TODO:

            return B32_FALSE;
        }
        
        resultCode = memory_map_alloc(&nextNodePtr->prevNodeKey, (void **)&prevNodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&nextNodePtr);
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&lhsListPtr);
            memory_unmap_alloc((void **)&rhsListPtr);

            // TODO:

            return B32_FALSE;
        }
        
        memcpy((void *)&nodePtr->prevNodeKey, &nextNodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->nextNodeKey, &prevNodePtr->nextNodeKey, sizeof(struct memory_allocation_key));

        memcpy((void *)&prevNodePtr->nextNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
        memcpy((void *)&nextNodePtr->prevNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));

        memory_unmap_alloc((void **)&nextNodePtr);
        memory_unmap_alloc((void **)&prevNodePtr);
    }
    else 
    {
        memcpy((void *)&nodePtr->prevNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->nextNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
    }
    
    ++rhsListPtr->activeListNodeCount;
    
    memcpy((void *)&rhsListPtr->activeNodeKeyList, nodeKeyPtr, sizeof(struct memory_allocation_key));

    memcpy((void *)&nodePtr->listKey, rhsListKeyPtr, sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&lhsListPtr);
    memory_unmap_alloc((void **)&rhsListPtr);

    _basic_list_assign_id(rhsListKeyPtr, nodeKeyPtr);

    return B32_TRUE;
}

b32
basic_list_move_free_node(const struct memory_allocation_key *lhsListKeyPtr,
    const struct memory_allocation_key *rhsListKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(lhsListKeyPtr)))
    {
        if (outNodeKeyPtr)
        {
            memory_get_null_allocation_key(outNodeKeyPtr);
        }

        return B32_TRUE;
    }

    if ((MEMORY_IS_ALLOCATION_NULL(rhsListKeyPtr)))
    {
        if (outNodeKeyPtr)
        {
            memory_get_null_allocation_key(outNodeKeyPtr);
        }

        return B32_TRUE;
    }

    if (!outNodeKeyPtr)
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key nodeKey;
    struct basic_list_node *nodePtr;

    if ((_basic_list_get_next_free_node(lhsListKeyPtr, &nodeKey)))
    {
        memory_error_code resultCode = memory_map_alloc(&nodeKey, 
        (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_get_null_allocation_key(outNodeKeyPtr);

            _basic_list_free_node(lhsListKeyPtr, &nodeKey);

            return B32_FALSE;
        }
    }
    else 
    {
        memory_get_null_allocation_key(outNodeKeyPtr);

        return B32_FALSE;
    }

    struct basic_list *lhsListPtr, *rhsListPtr;
    {
        memory_error_code resultCode = memory_map_alloc(lhsListKeyPtr, (void **)&lhsListPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)nodePtr);
            _basic_list_free_node(lhsListKeyPtr, &nodeKey);

            return B32_FALSE;
        }
        
        resultCode = memory_map_alloc(rhsListKeyPtr, (void **)&rhsListPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)lhsListPtr);
            memory_unmap_alloc((void **)nodePtr);
            _basic_list_free_node(lhsListKeyPtr, &nodeKey);

            return B32_FALSE;
        }
    }
    
    if (rhsListPtr->activeListNodeCount > 0)
    {
        struct basic_list_node *headNodePtr;

        memory_error_code resultCode = memory_map_alloc(&rhsListPtr->activeNodeKeyList, 
        (void **)&headNodePtr);
        
        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)nodePtr);
            _basic_list_free_node(lhsListKeyPtr, &nodeKey);
            memory_unmap_alloc((void **)lhsListPtr);
            memory_unmap_alloc((void **)rhsListPtr);

            return B32_FALSE;
        }
        
        struct basic_list_node *tailNodePtr;

        resultCode = memory_map_alloc(&headNodePtr->prevNodeKey, 
        (void **)&tailNodePtr);
        
        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)headNodePtr);
            memory_unmap_alloc((void **)nodePtr);
            _basic_list_free_node(lhsListKeyPtr, &nodeKey);
            memory_unmap_alloc((void **)lhsListPtr);
            memory_unmap_alloc((void **)rhsListPtr);

            return B32_FALSE;
        }

        memcpy((void *)&nodePtr->nextNodeKey, &rhsListPtr->activeNodeKeyList, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->prevNodeKey, &headNodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
        
        memcpy((void *)&tailNodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
        memcpy((void *)&headNodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));

        memory_unmap_alloc((void **)&tailNodePtr);
        memory_unmap_alloc((void **)&headNodePtr);
    }
    else 
    {
        memcpy((void *)&nodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
    }
    
    nodePtr->dataByteOffset = dataByteOffset;
    nodePtr->dataByteSize = dataByteSize;
    nodePtr->isActive = B32_TRUE;

    if (!(MEMORY_IS_ALLOCATION_NULL(dataKeyPtr)))
    {
        memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));
    }
    else 
    {
        memory_get_null_allocation_key(&nodePtr->dataKey);
    }

    memcpy((void *)&nodePtr->listKey, &rhsListKeyPtr, sizeof(struct memory_allocation_key));
    memcpy((void *)&rhsListPtr->activeNodeKeyList, &nodeKey, sizeof(struct memory_allocation_key));

    --lhsListPtr->freeListNodeCount;
    ++rhsListPtr->activeListNodeCount;

    memory_unmap_alloc((void **)nodePtr);
    memory_unmap_alloc((void **)lhsListPtr);
    memory_unmap_alloc((void **)rhsListPtr);
    
    memcpy((void *)outNodeKeyPtr, &nodeKey, sizeof(struct memory_allocation_key));
    
    _basic_list_assign_id(rhsListKeyPtr, &nodeKey);

    return B32_TRUE;
}

b32
basic_list_insert_before(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'listKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }
    
    if ((MEMORY_IS_ALLOCATION_NULL(lhsNodeKeyPtr)))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'lhsNodeKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    if (!outNodeKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'outNodeKeyPtr' argument cannot be NULL. Despite this, the argument/dereferenced value does not matter, "
            "as it is an out parameter. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    const struct memory_allocation_key nodeKey;
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode;

        if ((_basic_list_get_next_free_node(listKeyPtr, &nodeKey)))
        {
            resultCode = memory_map_alloc(&nodeKey, (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&nodePtr);

                return B32_FALSE;
            }
        }
        else
        {
            return B32_FALSE;
        }

        struct basic_list *listPtr;

        resultCode = memory_map_alloc(listKeyPtr, (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&nodePtr);
            _basic_list_free_node(listKeyPtr, &nodeKey);

            return B32_FALSE;
        }

        if (listPtr->activeListNodeCount > 0)
        {
            if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(listKeyPtr, lhsNodeKeyPtr)))
            {
                memory_unmap_alloc((void **)&listPtr);
                _basic_list_free_node(listKeyPtr, &nodeKey);

                return B32_FALSE;
            }

            struct basic_list_node *lhsNodePtr;

            resultCode = memory_map_alloc(lhsNodeKeyPtr, (void **)&lhsNodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&listPtr);
                _basic_list_free_node(listKeyPtr, &nodeKey);

                return B32_FALSE;
            }
            
            struct basic_list_node *prevNodePtr;

            resultCode = memory_map_alloc(&lhsNodePtr->prevNodeKey, (void **)&prevNodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&lhsNodePtr);
                memory_unmap_alloc((void **)&listPtr);
                _basic_list_free_node(listKeyPtr, &nodeKey);

                return B32_FALSE;
            }

            memcpy((void *)&nodePtr->prevNodeKey, &lhsNodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nodePtr->nextNodeKey, lhsNodeKeyPtr, sizeof(struct memory_allocation_key));
            
            memcpy((void *)&lhsNodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&prevNodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));

            if ((MEMORY_IS_ALLOCATION_KEY_EQUAL(&listPtr->activeNodeKeyList, lhsNodeKeyPtr)))
            {
                memcpy((void *)&listPtr->activeNodeKeyList, &nodeKey, sizeof(struct memory_allocation_key));
            }

            ++listPtr->activeListNodeCount;

            memory_unmap_alloc((void **)&prevNodePtr);
            memory_unmap_alloc((void **)&lhsNodePtr);
            memory_unmap_alloc((void **)&listPtr);
        }
        else 
        {
            memory_unmap_alloc((void **)&listPtr);
            _basic_list_free_node(listKeyPtr, &nodeKey);

            return B32_FALSE;
        }
    }

    nodePtr->dataByteOffset = dataByteOffset;
    nodePtr->dataByteSize = dataByteSize;
    nodePtr->isActive = B32_TRUE;

    if (!(MEMORY_IS_ALLOCATION_NULL(dataKeyPtr)))
    {
        memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));
    }
    else 
    {
        memory_get_null_allocation_key(&nodePtr->dataKey);
    }

    memcpy((void *)&nodePtr->listKey, listKeyPtr, sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&nodePtr);
    
    memcpy((void *)outNodeKeyPtr, &nodeKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
basic_list_free_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr)
{
    return _basic_list_free_node(listKeyPtr, nodeKeyPtr);
}

b32
basic_list_clear_nodes(const struct memory_allocation_key *listKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key headNodeKey;
    const struct memory_allocation_key currentNodeKey;

    if (!(basic_list_get_next_node(listKeyPtr, NULL, &headNodeKey)))
    {
        return B32_FALSE;
    }

    memcpy((void *)&headNodeKey, &currentNodeKey, sizeof(struct memory_allocation_key));

    do 
    {
        const struct memory_allocation_key nextNodeKey;

        if (!(basic_list_get_next_node(listKeyPtr, &currentNodeKey, &nextNodeKey)))
        {
            return B32_FALSE;
        }

        _basic_list_free_node(listKeyPtr, &currentNodeKey);

        memcpy((void *)&currentNodeKey, &nextNodeKey, sizeof(struct memory_allocation_key));

        if ((MEMORY_IS_ALLOCATION_KEY_EQUAL(&headNodeKey, &nextNodeKey)))
        {
            memory_get_null_allocation_key(&currentNodeKey);
        }
    } while (!(MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)));

    return B32_TRUE;
}

b32
basic_list_destroy(const struct memory_allocation_key *listKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(listKeyPtr)))
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key tailNodeKey;
    const struct memory_allocation_key currentNodeKey;

    if (!(basic_list_get_prev_node(listKeyPtr, NULL, &tailNodeKey)))
    {
        return B32_FALSE;
    }

    memcpy((void *)&currentNodeKey, &tailNodeKey, sizeof(struct memory_allocation_key));

    do 
    {
        const struct memory_allocation_key nextNodeKey;

        b32 nextNodeResult = basic_list_get_prev_node(listKeyPtr, &currentNodeKey, &nextNodeKey);

        memory_free(&currentNodeKey);

        if (!nextNodeResult)
        {
            break;
        }

        memcpy((void *)&currentNodeKey, &nextNodeKey, sizeof(struct memory_allocation_key));

        if ((MEMORY_IS_ALLOCATION_KEY_EQUAL(&tailNodeKey, &nextNodeKey)))
        {
            memory_get_null_allocation_key(&currentNodeKey);
        }
    } while (!(MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)));

    while ((_basic_list_get_next_free_node(listKeyPtr, &currentNodeKey)))
    {
        memory_free(&currentNodeKey);
    }

    memory_free(listKeyPtr);

    return B32_TRUE;
}