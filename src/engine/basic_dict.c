#include "basic_dict.h"
#include "types.h"
#include "utils.h"
#include "basic_list.h"
#include "memory.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct basic_dict_node
{
    const struct memory_allocation_key dictKey;
    const void *key;
    p64 databaseByteOffset;
    u64 databaseByteSize;
    b32 isActive;
};

struct basic_dict
{
    const struct memory_page_key pageKey;
    const struct memory_allocation_key bucketTableKey;
    const struct memory_allocation_key databaseArrKey;
    p64 databaseByteOffset;
    u64 databaseByteSize;
    const struct memory_allocation_key userPtrKey;
    u32 tableBucketCount;
    basic_dict_hash_func hashFunc;
    u64 keyByteSize;
};

static b32 
_default_hash_func(struct basic_dict *dictPtr, void *key, 
    struct utils_string_hash *outHashPtr)
{
    struct utils_string_hash resultHash;

    if (!(utils_generate_hash_from_string(key, &resultHash)))
    {
        return B32_FALSE;
    }

    memcpy(outHashPtr, &resultHash, sizeof(struct utils_string_hash));

    return B32_TRUE;
}

static b32
_basic_dict_remove_pair(const struct memory_allocation_key *dictKeyPtr, void *keyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(dictKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!keyPtr)
    {
        return B32_FALSE;
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    struct utils_string_hash tableHash;

    if (!(dictPtr->hashFunc(dictKeyPtr, keyPtr, &tableHash)))
    {
        memory_unmap_alloc((void **)&dictPtr);

        return B32_FALSE;
    }

    const struct memory_allocation_key *tablePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->tableKey, 
        (void **)&tablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }
    }

    const struct memory_allocation_key *bucketListKeyPtr = &tablePtr[dictPtr->tableBucketCount%tableHash.hash];

    u64 listNodeCount;

    if (!basic_list_get_active_count(bucketListKeyPtr, &listNodeCount))
    {
        return B32_FALSE;
    }

    if (listNodeCount < 1)
    {
        memory_unmap_alloc((void **)&tablePtr);
        memory_unmap_alloc((void **)&dictPtr);
        return B32_FALSE;
    }

    const struct memory_allocation_key headPairNodeKey;

    if (!(basic_list_get_next_node(bucketListKeyPtr, NULL, 
    &headPairNodeKey)))
    {
        memory_unmap_alloc((void **)&tablePtr);
        memory_unmap_alloc((void **)&dictPtr);

        return B32_FALSE;
    }

    const struct memory_allocation_key currentPairNodeKey;

    memcpy((void *)&currentPairNodeKey, &headPairNodeKey, sizeof(struct memory_allocation_key));

    do 
    {
        const struct memory_allocation_key *pairKeyPtr;

        if ((basic_list_get_data(&currentPairNodeKey, (void *)&pairKeyPtr)))
        {
            memory_unmap_alloc((void **)&tablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        struct basic_dict_node *pairPtr;
        
        memory_error_code resultCode = memory_map_alloc(pairKeyPtr, (void **)&pairPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&tablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }
        
        if (!(strcmp(pairPtr->key, keyPtr)))
        {
            memory_unmap_alloc((void **)&pairPtr);

            break;
        }

        const struct memory_allocation_key nextPairNodeKey;

        if (!(basic_list_get_next_node(&dictPtr->tableKey, &currentPairNodeKey, 
            &nextPairNodeKey)))
        {
            memory_unmap_alloc((void **)&pairPtr);
            memory_unmap_alloc((void **)&tablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            // TODO:

            return B32_FALSE;
        }

        if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&nextPairNodeKey, &headPairNodeKey)))
        {
            memcpy((void *)&currentPairNodeKey, &nextPairNodeKey, sizeof(struct memory_allocation_key));
        }
        else 
        {
            memory_get_null_allocation_key(&currentPairNodeKey);
        }

        memory_unmap_alloc((void **)&pairPtr);
    } while (!(MEMORY_IS_ALLOCATION_NULL(&currentPairNodeKey)));

    if (!(MEMORY_IS_ALLOCATION_NULL(&currentPairNodeKey)))
    {
        basic_list_move_node(bucketListKeyPtr, &dictPtr->freePairKeyList, &currentPairNodeKey);
    }
    else 
    {
        memory_unmap_alloc((void **)&tablePtr);
        memory_unmap_alloc((void **)&dictPtr);

        return B32_FALSE;
    }

    memory_unmap_alloc((void **)&tablePtr);
    memory_unmap_alloc((void **)&dictPtr);

    return B32_TRUE;
}

b32
basic_dict_create(const struct memory_page_key *memoryPageKeyPtr, basic_dict_hash_func hashFunc, 
    u32 initBucketCount, u64 keySize, const struct memory_allocation_key *userPtrKeyPtr,
    p64 databaseByteOffset, u64 databaseByteSize,
    const struct memory_allocation_key *databaseKeyPtr,
    const struct memory_allocation_key *outDictKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(databaseKeyPtr)))
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key dictKey;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_alloc(memoryPageKeyPtr, 
            sizeof(struct basic_dict) + sizeof(struct memory_raw_allocation_key), NULL, 
            &dictKey) != MEMORY_OK))
        {
            utils_fprintfln(stderr, "basic_dict_create(Line: %d): Cannot allocate basic_dict.\n",
                    __LINE__);

            return B32_FALSE;
        }
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode;
            
        if ((resultCode = memory_map_alloc(&dictKey, 
            (void **)&dictPtr)) != MEMORY_OK)
        {
            memory_free(&dictKey);

            utils_fprintfln(stderr, "basic_dict_create(Line: %d): Cannot map basic_dict allocation.",
                    __LINE__);

            return B32_FALSE;
        }
    }
    
    dictPtr->tableBucketCount = initBucketCount;

    memory_error_code resultCode = memory_alloc(memoryPageKeyPtr, 
        sizeof(struct memory_allocation_key)*initBucketCount, NULL, 
        &dictPtr->bucketTableKey);

    if (resultCode != MEMORY_OK)
    {
        return B32_FALSE;
    }

    memory_set_alloc_offset_width(&dictPtr->bucketTableKey, 0, 
        sizeof(struct memory_allocation_key)*initBucketCount, '\0');

    dictPtr->hashFunc = hashFunc ? hashFunc : &_default_hash_func;

    memcpy((struct memory_allocation_key*)&dictPtr->userPtrKey, userPtrKeyPtr, sizeof(struct memory_allocation_key));

    memcpy((void *)&dictPtr->databaseArrKey, databaseKeyPtr, sizeof(struct memory_allocation_key));

    dictPtr->databaseByteOffset = databaseByteOffset;
    dictPtr->databaseByteSize = databaseByteSize;

    memory_unmap_alloc((void **)&dictPtr);

    memcpy((void *)outDictKeyPtr, &dictKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
basic_dict_map_data(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    void **outDataPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(dictKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!keyPtr)
    {
        return B32_FALSE;
    }

    if (!outDataPtr)
    {
        return B32_FALSE;
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    const struct memory_allocation_key *bucketTablePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->bucketTableKey, 
        (void **)&bucketTablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }
    }
    
    struct utils_string_hash dictHash;

    dictPtr->hashFunc(dictPtr, keyPtr, &dictHash);

    const struct memory_allocation_key *listKeyPtr = &bucketTablePtr[dictPtr->tableBucketCount%dictHash.hash];

    const struct memory_allocation_key headNodeKey;

    if (!(basic_list_get_next_node(listKeyPtr, NULL, &headNodeKey)))
    {
        memory_unmap_alloc((void **)&bucketTablePtr);
        memory_unmap_alloc((void **)&dictPtr);
    }

    const struct memory_allocation_key currentNodeKey;

    memcpy((void *)&currentNodeKey, &headNodeKey, sizeof(struct memory_allocation_key));

    do 
    {
        struct basic_dict_node *nodePtr;

        if (!(basic_list_map_data(&currentNodeKey, (void **)&nodePtr)))
        {
            memory_unmap_alloc((void **)&bucketTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        if (!(memcmp(keyPtr, nodePtr->key, dictPtr->keyByteSize)))
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&bucketTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return basic_list_map_data(&currentNodeKey, (void **)outDataPtr);
        }

        const struct memory_allocation_key nextNodeKey;

        if (!(basic_list_get_next_node(listKeyPtr, &currentNodeKey, &nextNodeKey)))
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&bucketTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&nextNodeKey, &headNodeKey)))
        {
            memcpy((void *)&currentNodeKey, &nextNodeKey, sizeof(struct memory_allocation_key));
        }
        else
        {
            memory_get_null_allocation_key(&currentNodeKey);
        }

        memory_unmap_alloc((void **)&nodePtr);
    } while ((MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)));

    memory_unmap_alloc((void **)&bucketTablePtr);
    memory_unmap_alloc((void **)&dictPtr);

    return B32_FALSE;
}

b32
basic_dict_unmap_data(const struct memory_allocation_key *dictKeyPtr, void *keyPtr,
    void **outDataPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(dictKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!keyPtr)
    {
        return B32_FALSE;
    }

    if (!outDataPtr)
    {
        return B32_FALSE;
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    const struct memory_allocation_key *bucketTablePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->bucketTableKey, 
        (void **)&bucketTablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }
    }
    
    struct utils_string_hash dictHash;

    dictPtr->hashFunc(dictPtr, keyPtr, &dictHash);

    const struct memory_allocation_key *listKeyPtr = &bucketTablePtr[dictPtr->tableBucketCount%dictHash.hash];

    const struct memory_allocation_key headNodeKey;

    if (!(basic_list_get_next_node(listKeyPtr, NULL, &headNodeKey)))
    {
        memory_unmap_alloc((void **)&bucketTablePtr);
        memory_unmap_alloc((void **)&dictPtr);
    }

    const struct memory_allocation_key currentNodeKey;

    memcpy((void *)&currentNodeKey, &headNodeKey, sizeof(struct memory_allocation_key));

    do 
    {
        struct basic_dict_node *nodePtr;

        if (!(basic_list_map_data(&currentNodeKey, (void **)&nodePtr)))
        {
            memory_unmap_alloc((void **)&bucketTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        if (!(memcmp(keyPtr, nodePtr->key, dictPtr->keyByteSize)))
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&bucketTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return basic_list_unmap_data(listKeyPtr);
        }

        const struct memory_allocation_key nextNodeKey;

        if (!(basic_list_get_next_node(listKeyPtr, &currentNodeKey, &nextNodeKey)))
        {
            memory_unmap_alloc((void **)&nodePtr);
            memory_unmap_alloc((void **)&bucketTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&nextNodeKey, &headNodeKey)))
        {
            memcpy((void *)&currentNodeKey, &nextNodeKey, sizeof(struct memory_allocation_key));
        }
        else
        {
            memory_get_null_allocation_key(&currentNodeKey);
        }

        memory_unmap_alloc((void **)&nodePtr);
    } while ((MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)));

    memory_unmap_alloc((void **)&bucketTablePtr);
    memory_unmap_alloc((void **)&dictPtr);

    return B32_FALSE;
}

b32
basic_dict_set_data_info(const struct memory_allocation_key *dictKeyPtr,
    void *keyPtr, p64 *databaseByteoffset, u64 *databaseByteSize);

b32
basic_dict_remove(const struct memory_allocation_key *dictKeyPtr, void *keyPtr)
{
    return _basic_dict_remove_pair(dictKeyPtr, keyPtr);
}

b32
basic_dict_clear(const struct memory_allocation_key *dictKeyPtr)
{
    return B32_TRUE;
}

b32
basic_dict_get_is_found(const struct memory_allocation_key *dictKeyPtr, void *keyPtr)
{
    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            // TODO:

            return B32_FALSE;
        }
    }

    struct utils_string_hash tableHash;

    if (!dictPtr->hashFunc(dictKeyPtr, keyPtr, &tableHash))
    {
        memory_unmap_alloc((void **)&dictPtr);

        return B32_FALSE;
    }

    const struct memory_allocation_key *tablePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->tableKey, 
        (void **)&tablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictPtr);

            // TODO:

            return B32_FALSE;
        }
    }

    const struct memory_allocation_key *listKeyPtr = &tablePtr[dictPtr->tableBucketCount%tableHash.hash];

    struct memory_allocation_key headTableListPairNodeKey;

    if (!(basic_list_get_next_node(listKeyPtr, NULL, &headTableListPairNodeKey)))
    {
        memory_unmap_alloc((void **)&tablePtr);
        memory_unmap_alloc((void **)&dictPtr);

        // TODO:

        return B32_FALSE;
    }

    const struct memory_allocation_key tableListPairNodeKey;

    memcpy((void *)&tableListPairNodeKey, &headTableListPairNodeKey, 
    sizeof(struct memory_allocation_key));

    do 
    {
        struct basic_dict_pair *pairPtr;

        memory_error_code resultCode = memory_map_alloc(&tableListPairNodeKey, 
        (void *)&pairPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&tablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            // TODO:

            return B32_FALSE;
        }

        if (!strcmp(pairPtr->key, keyPtr))
        {
            return B32_TRUE;
        }

        const struct memory_allocation_key nextTableListPairKeyNode;

        basic_list_get_next_node(listKeyPtr, &tableListPairNodeKey, 
        &nextTableListPairKeyNode);

        if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&nextTableListPairKeyNode, &headTableListPairNodeKey)))
        {
            memcpy((void *)&tableListPairNodeKey, &nextTableListPairKeyNode,
            sizeof(struct memory_allocation_key));
        }
        else 
        {
            return B32_FALSE;
        }
    } while (!(MEMORY_IS_ALLOCATION_NULL(&tableListPairNodeKey)));

    memory_unmap_alloc((void **)&tablePtr);
    memory_unmap_alloc((void **)&dictPtr);

    return B32_TRUE;
}