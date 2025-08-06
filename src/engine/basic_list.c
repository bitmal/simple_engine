#include "basic_list.h"
#include "memory.h"
#include "basic_list.h"
#include "utils.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct basic_list_node
{
    const struct memory_allocation_key listKey;
    const struct memory_allocation_key dataKey;
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
basic_list_create(const struct memory_page_key *memoryPageKeyPtr, 
    const struct memory_allocation_key *outListKeyPtr)
{
    if (!memoryPageKeyPtr)
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

    memcpy((void *)outListKeyPtr, (void *)&listKey, 
        sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&listPtr);

    return B32_TRUE;
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
basic_list_get_data(const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outDataKeyPtr)
{
    if (MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr) || !outDataKeyPtr)
    {
        if (outDataKeyPtr)
        {
            ((struct memory_allocation_key *)outDataKeyPtr)->allocId = MEMORY_INT_ID_NULL;
            ((struct memory_context_key *)&((struct memory_allocation_key *)outDataKeyPtr)->
                contextKey)->contextId = MEMORY_SHORT_ID_NULL;
        }

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

    if (nodePtr->isActive)
    {
        memcpy((void *)outDataKeyPtr, &nodePtr->dataKey, sizeof(struct memory_allocation_key));
    }
    else 
    {
        // TODO:
        memory_get_null_allocation_key(outDataKeyPtr);

        return B32_FALSE;
    }

    return B32_TRUE;
}

b32
basic_list_set_data(const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *dataKeyPtr)
{
    if (MEMORY_IS_ALLOCATION_NULL(nodeKeyPtr))
    {
        return B32_FALSE;
    }

    if (MEMORY_IS_ALLOCATION_NULL(dataKeyPtr))
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

    if (nodePtr->isActive)
    {
        memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));
    }
    else 
    {
        // TODO:

        return B32_FALSE;
    }

    return B32_TRUE;
}

b32
basic_list_append(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (MEMORY_IS_ALLOCATION_NULL(listKeyPtr))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'listKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    if (dataKeyPtr && ((dataKeyPtr->allocId == MEMORY_INT_ID_NULL) || 
        (dataKeyPtr->contextKey.contextId == MEMORY_INT_ID_NULL)))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'dataKeyPtr' memory key cannot be NULL, despite the pointer itself being safe to be NULL. Aborting.", 
            __func__, __LINE__);

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
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, 
        (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): "
                "Could not memory map list. Aborting.", __func__, __LINE__);

            return B32_FALSE;
        }
    }

    const struct memory_allocation_key nodeKey;
    struct basic_list_node *nodePtr;
    {
        b32 isNewNode;
        memory_error_code resultCode = MEMORY_OK;

        if (listPtr->freeListNodeCount > 0)
        {
            resultCode = memory_map_alloc(&listPtr->freeNodeKeyList, 
            (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&listPtr);

                utils_fprintfln(stderr, "%s(Line: %d): "
                    "Could not memory map list node. Aborting.", __func__, __LINE__);

                return B32_FALSE;
            }

            if (listPtr->freeListNodeCount > 1)
            {
                struct basic_list_node *prevNodePtr;
                {
                    resultCode = memory_map_alloc(&nodePtr->prevNodeKey, 
                    (void **)&prevNodePtr);

                    if (resultCode != MEMORY_OK)
                    {
                        memory_unmap_alloc((void **)&nodePtr);
                        memory_free(&nodeKey);
                        memory_unmap_alloc((void **)&listPtr);

                        utils_fprintfln(stderr, "%s(Line: %d): "
                            "Could not memory map an important list node. Aborting.", __func__, __LINE__);

                        return B32_FALSE;
                    }
                }
                
                struct basic_list_node *nextNodePtr;
                {
                    resultCode = memory_map_alloc(&nodePtr->nextNodeKey, 
                    (void **)&nextNodePtr);

                    if (resultCode != MEMORY_OK)
                    {
                        memory_unmap_alloc((void **)prevNodePtr);
                        memory_unmap_alloc((void **)&nodePtr);
                        memory_free(&nodeKey);
                        memory_unmap_alloc((void **)&listPtr);

                        utils_fprintfln(stderr, "%s(Line: %d): "
                            "Could not memory map an important list node. Aborting.", __func__, __LINE__);

                        return B32_FALSE;
                    }
                }

                memcpy((void *)&prevNodePtr->nextNodeKey, &nodePtr->nextNodeKey, 
                sizeof(struct memory_allocation_key));
                
                memcpy((void *)&nextNodePtr->prevNodeKey, &nodePtr->prevNodeKey, 
                sizeof(struct memory_allocation_key));
                
                memcpy((void *)&listPtr->freeNodeKeyList, &nodePtr->nextNodeKey, 
                sizeof(struct memory_allocation_key));

                memory_unmap_alloc((void **)&prevNodePtr);
                memory_unmap_alloc((void **)&nextNodePtr);
            }
            else 
            {
                memset((void *)&listPtr->freeNodeKeyList, 0, sizeof(struct memory_allocation_key));
            }
            
            memcpy((void *)&nodeKey, &listPtr->freeNodeKeyList, 
            sizeof(struct memory_allocation_key));

            isNewNode = B32_FALSE;
        }
        else 
        {
            resultCode = memory_alloc(&listPtr->memoryPageKey, 
            sizeof(struct basic_list_node), NULL, 
            &nodeKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&listPtr);

                utils_fprintfln(stderr, "%s(Line: %d): "
                    "Could not memory map list node. Aborting.", __func__, __LINE__);

                return B32_FALSE;
            }

            resultCode = memory_map_alloc(&nodeKey, (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_free(&nodeKey);
                memory_unmap_alloc((void **)&listPtr);

                utils_fprintfln(stderr, "%s(Line: %d): "
                    "Could not memory map list node. Aborting.", __func__, __LINE__);

                return B32_FALSE;
            }

            memcpy((void *)&nodePtr->prevNodeKey, &nodeKey, sizeof(nodeKey));
            memcpy((void *)&nodePtr->nextNodeKey, &nodeKey, sizeof(nodeKey));

            isNewNode = B32_TRUE;
        }

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&listPtr);

            if (isNewNode)
            {
                // TODO: add node to free list instead
                nodePtr->isActive = B32_FALSE;
                memory_free(&nodeKey);
            }
            
            utils_fprintfln(stderr, "%s(Line: %d): "
                "Could not memory map the allocated list node. Aborting.", __func__, __LINE__);

            return B32_FALSE;
        }

        if (listPtr->activeListNodeCount > 0)
        {
            struct basic_list_node *activeHeadNode;

            memory_error_code resultCode = memory_map_alloc(&listPtr->activeNodeKeyList, 
            (void **)&activeHeadNode);

            if (resultCode != MEMORY_OK)
            {
                // TODO: place list node (back) into free list cause something went wrong with mapping 
                // the current head node of the active list
                // TODO: unmap and free other stuff above lol

                nodePtr->isActive = B32_FALSE;
                
                return B32_FALSE;
            }

            struct basic_list_node *prevNode;

            resultCode = memory_map_alloc(&activeHeadNode->prevNodeKey, (void **)&prevNode);
            
            if (resultCode != MEMORY_OK)
            {
                // TODO:

                return B32_FALSE;
            }

            memcpy((void *)&nodePtr->nextNodeKey, &listPtr->activeNodeKeyList, sizeof(struct memory_allocation_key));
            memcpy((void *)&nodePtr->prevNodeKey, &activeHeadNode->prevNodeKey, sizeof(struct memory_allocation_key));

            memcpy((void *)&prevNode->nextNodeKey, &listPtr->activeNodeKeyList, sizeof(struct memory_allocation_key));
            memcpy((void *)&activeHeadNode->prevNodeKey, &listPtr->activeNodeKeyList, sizeof(struct memory_allocation_key));
            
            memory_unmap_alloc((void **)&prevNode);
            memory_unmap_alloc((void **)&activeHeadNode);
        }
        else 
        {
            memcpy((void *)&nodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));

            memcpy((void *)&listPtr->activeNodeKeyList, &nodeKey, sizeof(struct memory_allocation_key));
        }

        if (isNewNode)
        {
            --listPtr->freeListNodeCount;
        }

        ++listPtr->activeListNodeCount;
    }

    if (!MEMORY_IS_ALLOCATION_NULL(dataKeyPtr))
    {
        memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));
    }
    else 
    {
        memset((void *)&nodePtr->dataKey, '\0', sizeof(struct memory_allocation_key));
    }
    
    nodePtr->isActive = B32_TRUE;

    memcpy((void *)outNodeKeyPtr, &nodeKey, sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

    return B32_TRUE;
}

b32
basic_list_insert_front(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (MEMORY_IS_ALLOCATION_NULL(listKeyPtr))
    {
        // TODO:

        return B32_FALSE;
    }

    if (MEMORY_IS_ALLOCATION_NULL(dataKeyPtr))
    {
        // TODO:

        return B32_FALSE;
    }

    if (!outNodeKeyPtr)
    {
        // TODO:

        return B32_FALSE;
    }

    struct basic_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, 
        (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }
    }

    const struct memory_allocation_key nodeAllocationKey;
    struct basic_list_node *nodePtr;

    if (listPtr->freeListNodeCount > 0)
    {
        memcpy((void *)&nodeAllocationKey, &listPtr->freeNodeKeyList, sizeof(struct memory_allocation_key));

        memory_error_code resultCode = memory_map_alloc(&nodeAllocationKey, (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }

        if (listPtr->freeListNodeCount > 1)
        {
            struct basic_list_node *prevNodePtr;

            memory_error_code resultCode = memory_map_alloc(&nodePtr->prevNodeKey, 
            (void **)&prevNodePtr);

            if (resultCode != MEMORY_OK)
            {
                // TODO:

                return B32_FALSE;
            }

            struct basic_list_node *nextNodePtr;

            resultCode = memory_map_alloc(&nodePtr->nextNodeKey,
            (void **)nextNodePtr);
            
            if (resultCode != MEMORY_OK)
            {
                // TODO:

                return B32_FALSE;
            }
            
            memcpy((void *)&listPtr->freeNodeKeyList, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));

            memcpy((void *)&prevNodePtr->nextNodeKey, &nodeAllocationKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nextNodePtr->prevNodeKey, &nodeAllocationKey, sizeof(struct memory_allocation_key));
        }
        else 
        {
            struct basic_list_node *prevNodePtr;

            memory_error_code resultCode = memory_map_alloc(&nodePtr->prevNodeKey, 
            (void **)&prevNodePtr);

            if (resultCode != MEMORY_OK)
            {
                // TODO:

                return B32_FALSE;
            }

            struct basic_list_node *nextNodePtr;

            resultCode = memory_map_alloc(&nodePtr->nextNodeKey,
            (void **)nextNodePtr);
            
            if (resultCode != MEMORY_OK)
            {
                // TODO:

                return B32_FALSE;
            }

            memcpy((void *)&prevNodePtr->nextNodeKey, &nodeAllocationKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nextNodePtr->prevNodeKey, &nodeAllocationKey, sizeof(struct memory_allocation_key));
        }
        
        --listPtr->freeListNodeCount;
    }
    else 
    {
        memory_error_code resultCode = memory_alloc(&listPtr->memoryPageKey, 
        sizeof(struct basic_list_node), NULL, 
        &nodeAllocationKey);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&nodeAllocationKey, (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }
    }

    if (listPtr->activeListNodeCount > 0)
    {
        struct basic_list_node *nextNodePtr;

        memory_error_code resultCode = memory_map_alloc(&listPtr->activeNodeKeyList, 
        (void **)&nextNodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:
            nodePtr->isActive = B32_FALSE;

            return B32_FALSE;
        }

        struct basic_list_node *prevNodePtr;
        
        resultCode = memory_map_alloc(&nextNodePtr->prevNodeKey, 
        (void **)&prevNodePtr);

        memcpy((void *)&nodePtr->prevNodeKey, &nextNodePtr->prevNodeKey, 
        sizeof(struct memory_allocation_key));
        
        memcpy((void *)&nodePtr->nextNodeKey, &listPtr->activeNodeKeyList, 
        sizeof(struct memory_allocation_key));

        memcpy((void *)&nextNodePtr->prevNodeKey, &nodeAllocationKey, 
        sizeof(struct memory_allocation_key));

        memcpy((void *)&prevNodePtr->nextNodeKey, &nodeAllocationKey, 
        sizeof(struct memory_allocation_key));

        memory_unmap_alloc((void **)&prevNodePtr);
        memory_unmap_alloc((void **)&nextNodePtr);
    }
    else 
    {
        memcpy((void *)&nodePtr->prevNodeKey, &nodeAllocationKey, 
        sizeof(struct memory_allocation_key));
        
        memcpy((void *)&nodePtr->nextNodeKey, &nodeAllocationKey, 
        sizeof(struct memory_allocation_key));
    }

    memcpy((void *)&listPtr->activeNodeKeyList, &nodeAllocationKey, 
    sizeof(struct memory_allocation_key));
    
    memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));

    ++listPtr->activeListNodeCount;
    
    nodePtr->isActive = B32_TRUE;

    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

    memcpy((void *)outNodeKeyPtr, &nodeAllocationKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
basic_list_append_after(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (MEMORY_IS_ALLOCATION_NULL(listKeyPtr))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'listKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }
    
    if (MEMORY_IS_ALLOCATION_NULL(lhsNodeKeyPtr))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'lhsNodeKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }
    
    if (MEMORY_IS_ALLOCATION_NULL(dataKeyPtr))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'dataKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }
    
    if (!outNodeKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'outRhsNodeKeyPtr' argument cannot be NULL. Despite this, the argument/dereferenced value does not matter, "
            "as it is an out parameter. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    struct basic_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, 
        (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): "
                "Could not memory map list. Aborting.", __func__, __LINE__);

            return B32_FALSE;
        }
    }

    struct basic_list_node *lhsNodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(lhsNodeKeyPtr, (void **)&lhsNodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }

        if (!lhsNodePtr)
        {
            // TODO:

            return B32_FALSE;
        }
    }

    const struct memory_allocation_key nodeKey;
    struct basic_list_node *nodePtr;
    {
        b32 isNewNode;
        memory_error_code resultCode = MEMORY_OK;

        if (listPtr->freeListNodeCount > 0)
        {
            resultCode = memory_map_alloc(&listPtr->freeNodeKeyList, 
            (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&listPtr);

                utils_fprintfln(stderr, "%s(Line: %d): "
                    "Could not memory map list node. Aborting.", __func__, __LINE__);

                return B32_FALSE;
            }

            if (listPtr->freeListNodeCount > 1)
            {
                struct basic_list_node *prevNodePtr;
                {
                    resultCode = memory_map_alloc(&nodePtr->prevNodeKey, 
                    (void **)&prevNodePtr);

                    if (resultCode != MEMORY_OK)
                    {
                        memory_unmap_alloc((void **)&nodePtr);
                        memory_free(&nodeKey);
                        memory_unmap_alloc((void **)&listPtr);

                        utils_fprintfln(stderr, "%s(Line: %d): "
                            "Could not memory map an important list node. Aborting.", __func__, __LINE__);

                        return B32_FALSE;
                    }
                }
                
                struct basic_list_node *nextNodePtr;
                {
                    resultCode = memory_map_alloc(&nodePtr->nextNodeKey, 
                    (void **)&nextNodePtr);

                    if (resultCode != MEMORY_OK)
                    {
                        memory_unmap_alloc((void **)prevNodePtr);
                        memory_unmap_alloc((void **)&nodePtr);
                        memory_free(&nodeKey);
                        memory_unmap_alloc((void **)&listPtr);

                        utils_fprintfln(stderr, "%s(Line: %d): "
                            "Could not memory map an important list node. Aborting.", __func__, __LINE__);

                        return B32_FALSE;
                    }
                }

                memcpy((void *)&prevNodePtr->nextNodeKey, &nodePtr->nextNodeKey, 
                sizeof(struct memory_allocation_key));
                
                memcpy((void *)&nextNodePtr->prevNodeKey, &nodePtr->prevNodeKey, 
                sizeof(struct memory_allocation_key));
                
                memcpy((void *)&listPtr->freeNodeKeyList, &nodePtr->nextNodeKey, 
                sizeof(struct memory_allocation_key));

                memory_unmap_alloc((void **)&prevNodePtr);
                memory_unmap_alloc((void **)&nextNodePtr);
            }
            else 
            {
                memset((void *)&listPtr->freeNodeKeyList, 0, sizeof(struct memory_allocation_key));
            }
            
            memcpy((void *)&nodeKey, &listPtr->freeNodeKeyList, 
            sizeof(struct memory_allocation_key));

            isNewNode = B32_FALSE;
        }
        else 
        {
            resultCode = memory_alloc(&listPtr->memoryPageKey, 
            sizeof(struct basic_list_node), NULL, 
            &nodeKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&listPtr);

                utils_fprintfln(stderr, "%s(Line: %d): "
                    "Could not memory map list node. Aborting.", __func__, __LINE__);

                return B32_FALSE;
            }

            resultCode = memory_map_alloc(&nodeKey, (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_free(&nodeKey);
                memory_unmap_alloc((void **)&listPtr);

                utils_fprintfln(stderr, "%s(Line: %d): "
                    "Could not memory map list node. Aborting.", __func__, __LINE__);

                return B32_FALSE;
            }

            memcpy((void *)&nodePtr->prevNodeKey, &nodeKey, sizeof(nodeKey));
            memcpy((void *)&nodePtr->nextNodeKey, &nodeKey, sizeof(nodeKey));

            isNewNode = B32_TRUE;
        }

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&listPtr);

            if (isNewNode)
            {
                // TODO: add node to free list instead
                memory_free(&nodeKey);
            }
            
            utils_fprintfln(stderr, "%s(Line: %d): "
                "Could not memory map the allocated list node. Aborting.", __func__, __LINE__);

            return B32_FALSE;
        }

        if (listPtr->activeListNodeCount > 0)
        {
            struct basic_list_node *nextNodePtr;
            
            resultCode = memory_map_alloc(&lhsNodePtr->nextNodeKey, (void **)&nextNodePtr);

            if (resultCode != MEMORY_OK)
            {
                // TODO: place list node (back) into free list cause something went wrong with mapping 
                // the current head node of the active list
                // TODO: unmap and free other stuff above lol

                nodePtr->isActive = B32_FALSE;
                
                return B32_FALSE;
            }

            memcpy((void *)&nodePtr->prevNodeKey, lhsNodeKeyPtr, sizeof(struct memory_allocation_key));
            memcpy((void *)&nodePtr->nextNodeKey, &lhsNodePtr->nextNodeKey, sizeof(struct memory_allocation_key));

            memcpy((void *)&lhsNodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nextNodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));

            memory_unmap_alloc((void **)&nextNodePtr);
        }
        else 
        {
            memcpy((void *)&nodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));

            memcpy((void *)&listPtr->activeNodeKeyList, &nodeKey, sizeof(struct memory_allocation_key));
        }

        if (isNewNode)
        {
            --listPtr->freeListNodeCount;
        }

        ++listPtr->activeListNodeCount;
    }

    memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));
    
    nodePtr->isActive = B32_TRUE;

    memory_unmap_alloc((void **)&lhsNodePtr);
    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

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
    
    memcpy((void *)&rhsListPtr->activeNodeKeyList, nodeKeyPtr, sizeof(struct memory_allocation_key));

    ++rhsListPtr->activeListNodeCount;

    memcpy((void *)&nodePtr->listKey, rhsListKeyPtr, sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&lhsListPtr);
    memory_unmap_alloc((void **)&rhsListPtr);

    return B32_TRUE;
}

b32
basic_list_insert_before(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr)
{
    if (MEMORY_IS_ALLOCATION_NULL(listKeyPtr))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'listKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }
    
    if (MEMORY_IS_ALLOCATION_NULL(lhsNodeKeyPtr))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'lhsNodeKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }
    
    if (MEMORY_IS_ALLOCATION_NULL(dataKeyPtr))
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'dataKeyPtr' argument cannot be NULL, nor the key itself. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }
    
    if (!outNodeKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): "
            "'outRhsNodeKeyPtr' argument cannot be NULL. Despite this, the argument/dereferenced value does not matter, "
            "as it is an out parameter. Aborting.", __func__, __LINE__);

        return B32_FALSE;
    }

    struct basic_list *listPtr;
    {
        memory_error_code resultCode = memory_map_alloc(listKeyPtr, 
        (void **)&listPtr);

        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): "
                "Could not memory map list. Aborting.", __func__, __LINE__);

            return B32_FALSE;
        }
    }

    struct basic_list_node *lhsNodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(lhsNodeKeyPtr, (void **)&lhsNodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }

        if (!lhsNodePtr->isActive)
        {
            // TODO:

            return B32_FALSE;
        }
    }

    const struct memory_allocation_key nodeKey;
    struct basic_list_node *nodePtr;
    {
        b32 isNewNode;
        memory_error_code resultCode = MEMORY_OK;

        if (listPtr->freeListNodeCount > 0)
        {
            resultCode = memory_map_alloc(&listPtr->freeNodeKeyList, 
            (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&listPtr);

                utils_fprintfln(stderr, "%s(Line: %d): "
                    "Could not memory map list node. Aborting.", __func__, __LINE__);

                return B32_FALSE;
            }

            if (listPtr->freeListNodeCount > 1)
            {
                struct basic_list_node *prevNodePtr;
                {
                    resultCode = memory_map_alloc(&nodePtr->prevNodeKey, 
                    (void **)&prevNodePtr);

                    if (resultCode != MEMORY_OK)
                    {
                        memory_unmap_alloc((void **)&nodePtr);
                        memory_free(&nodeKey);
                        memory_unmap_alloc((void **)&listPtr);

                        utils_fprintfln(stderr, "%s(Line: %d): "
                            "Could not memory map an important list node. Aborting.", __func__, __LINE__);

                        return B32_FALSE;
                    }
                }
                
                struct basic_list_node *nextNodePtr;
                {
                    resultCode = memory_map_alloc(&nodePtr->nextNodeKey, 
                    (void **)&nextNodePtr);

                    if (resultCode != MEMORY_OK)
                    {
                        memory_unmap_alloc((void **)prevNodePtr);
                        memory_unmap_alloc((void **)&nodePtr);
                        memory_free(&nodeKey);
                        memory_unmap_alloc((void **)&listPtr);

                        utils_fprintfln(stderr, "%s(Line: %d): "
                            "Could not memory map an important list node. Aborting.", __func__, __LINE__);

                        return B32_FALSE;
                    }
                }

                memcpy((void *)&prevNodePtr->nextNodeKey, &nodePtr->nextNodeKey, 
                sizeof(struct memory_allocation_key));
                
                memcpy((void *)&nextNodePtr->prevNodeKey, &nodePtr->prevNodeKey, 
                sizeof(struct memory_allocation_key));
                
                memcpy((void *)&listPtr->freeNodeKeyList, &nodePtr->nextNodeKey, 
                sizeof(struct memory_allocation_key));

                memory_unmap_alloc((void **)&prevNodePtr);
                memory_unmap_alloc((void **)&nextNodePtr);
            }
            else 
            {
                memset((void *)&listPtr->freeNodeKeyList, 0, sizeof(struct memory_allocation_key));
            }
            
            memcpy((void *)&nodeKey, &listPtr->freeNodeKeyList, 
            sizeof(struct memory_allocation_key));

            isNewNode = B32_FALSE;
        }
        else 
        {
            resultCode = memory_alloc(&listPtr->memoryPageKey, 
            sizeof(struct basic_list_node), NULL, 
            &nodeKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&listPtr);

                utils_fprintfln(stderr, "%s(Line: %d): "
                    "Could not memory map list node. Aborting.", __func__, __LINE__);

                return B32_FALSE;
            }

            resultCode = memory_map_alloc(&nodeKey, (void **)&nodePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_free(&nodeKey);
                memory_unmap_alloc((void **)&listPtr);

                utils_fprintfln(stderr, "%s(Line: %d): "
                    "Could not memory map list node. Aborting.", __func__, __LINE__);

                return B32_FALSE;
            }

            memcpy((void *)&nodePtr->prevNodeKey, &nodeKey, sizeof(nodeKey));
            memcpy((void *)&nodePtr->nextNodeKey, &nodeKey, sizeof(nodeKey));

            isNewNode = B32_TRUE;
        }

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&listPtr);

            if (isNewNode)
            {
                // TODO: add node to free list instead
                memory_free(&nodeKey);
            }
            
            utils_fprintfln(stderr, "%s(Line: %d): "
                "Could not memory map the allocated list node. Aborting.", __func__, __LINE__);

            return B32_FALSE;
        }

        if (listPtr->activeListNodeCount > 0)
        {
            struct basic_list_node *prevNodePtr;
            
            resultCode = memory_map_alloc(&lhsNodePtr->prevNodeKey, (void **)&prevNodePtr);

            if (resultCode != MEMORY_OK)
            {
                // TODO: place list node (back) into free list cause something went wrong with mapping 
                // the current head node of the active list
                // TODO: unmap and free other stuff above lol

                nodePtr->isActive = B32_FALSE;
                
                return B32_FALSE;
            }

            memcpy((void *)&nodePtr->prevNodeKey, &lhsNodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nodePtr->nextNodeKey, lhsNodeKeyPtr, sizeof(struct memory_allocation_key));

            memcpy((void *)&lhsNodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&prevNodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));

            memory_unmap_alloc((void **)&prevNodePtr);

            if ((MEMORY_IS_ALLOCATION_KEY_EQUAL((&listPtr->activeNodeKeyList), lhsNodeKeyPtr)))
            {
                memcpy((void *)&listPtr->activeNodeKeyList, &nodeKey, sizeof(struct memory_allocation_key));
            }
        }
        else 
        {
            memcpy((void *)&nodePtr->nextNodeKey, &nodeKey, sizeof(struct memory_allocation_key));
            memcpy((void *)&nodePtr->prevNodeKey, &nodeKey, sizeof(struct memory_allocation_key));

            memcpy((void *)&listPtr->activeNodeKeyList, &nodeKey, sizeof(struct memory_allocation_key));
        }

        if (isNewNode)
        {
            --listPtr->freeListNodeCount;
        }

        ++listPtr->activeListNodeCount;
    }

    memcpy((void *)&nodePtr->dataKey, dataKeyPtr, sizeof(struct memory_allocation_key));

    nodePtr->isActive = B32_TRUE;

    memory_unmap_alloc((void **)&lhsNodePtr);
    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

    memcpy((void *)outNodeKeyPtr, &nodeKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
basic_list_remove_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key **nodeKeyPtr)
{
    if (MEMORY_IS_ALLOCATION_NULL(listKeyPtr))
    {
        // TODO:

        return B32_FALSE;
    }

    if (!nodeKeyPtr)
    {
        // TODO:

        return B32_FALSE;
    }

    if (MEMORY_IS_ALLOCATION_NULL((*nodeKeyPtr)))
    {
        // TODO:

        return B32_FALSE;
    }
    
    struct basic_list_node *nodePtr;
    {
        memory_error_code resultCode = memory_map_alloc(*nodeKeyPtr, (void **)&nodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }

        if (!nodePtr->isActive)
        {
            // TODO:

            return B32_FALSE;
        }
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

    if (listPtr->activeListNodeCount > 1)
    {
        struct basic_list_node *prevNodePtr;

        memory_error_code resultCode = memory_map_alloc(&nodePtr->prevNodeKey, (void **)&prevNodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }
        
        struct basic_list_node *nextNodePtr;

        resultCode = memory_map_alloc(&nodePtr->nextNodeKey, (void **)&nextNodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }

        memcpy((void *)&prevNodePtr->nextNodeKey, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));
        memcpy((void *)&nextNodePtr->prevNodeKey, &nodePtr->prevNodeKey, sizeof(struct memory_allocation_key));

        memory_unmap_alloc((void **)&prevNodePtr);
        memory_unmap_alloc((void **)&nextNodePtr);
        
        if ((MEMORY_IS_ALLOCATION_KEY_EQUAL((*nodeKeyPtr), (&listPtr->activeNodeKeyList))))
        {
            memcpy((void *)&listPtr->activeNodeKeyList, &nodePtr->nextNodeKey, sizeof(struct memory_allocation_key));
        }
    }
    else 
    {
        memory_get_null_allocation_key(&listPtr->activeNodeKeyList);
    }

    --listPtr->activeListNodeCount;

    if (listPtr->freeListNodeCount > 0)
    {
        struct basic_list_node *nextNodePtr;

        memory_error_code resultCode = memory_map_alloc(&listPtr->freeNodeKeyList, (void **)&nextNodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }
        
        struct basic_list_node *prevNodePtr;

        resultCode = memory_map_alloc(&nextNodePtr->prevNodeKey, (void **)&prevNodePtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }

        memcpy((void *)&nodePtr->prevNodeKey, &nextNodePtr->prevNodeKey, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->nextNodeKey, &listPtr->freeNodeKeyList, sizeof(struct memory_allocation_key));

        memcpy((void *)&nextNodePtr->prevNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
        memcpy((void *)&prevNodePtr->nextNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
    }
    else 
    {
        memcpy((void *)&nodePtr->prevNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
        memcpy((void *)&nodePtr->nextNodeKey, nodeKeyPtr, sizeof(struct memory_allocation_key));
    }
    
    memcpy((void *)&listPtr->freeNodeKeyList, nodeKeyPtr, sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&nodePtr);
    memory_unmap_alloc((void **)&listPtr);

    memory_get_null_allocation_key((void *)nodeKeyPtr);

    return B32_TRUE;
}