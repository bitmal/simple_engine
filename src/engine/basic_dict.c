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
    const struct memory_raw_allocation_key rawKeyKey;
    p64 dataByteOffset;
    u64 dataByteSize;
    const struct memory_allocation_key dataKey;
    b32 isActive;
};

struct basic_dict
{
    const struct memory_page_key pageKey;
    const struct memory_allocation_key bucketTableKey;
    const struct memory_allocation_key freeNodeList;
    const struct memory_allocation_key userPtrKey;
    u32 tableBucketCount;
    basic_dict_hash_func hashFunc;
    basic_dict_create_key_copy_func keyCopyFunc;
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

b32
_default_key_copy_func(struct basic_dict *dictPtr, void *keyPtr, 
    const struct memory_raw_allocation_key *outRawKeyKeyPtr)
{
    if (!dictPtr)
    {
        return B32_FALSE;
    }

    if (!keyPtr)
    {
        return B32_FALSE;
    }

    size_t keySize = strlen(keyPtr) + 1;

    const struct memory_raw_allocation_key rawKeyKey;
    void *rawKeyPtr;
    {
        memory_error_code resultCode = memory_raw_alloc(&rawKeyKey, 
        keySize);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_raw_allocation(&rawKeyKey, (void **)&rawKeyPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    memcpy(rawKeyPtr, keyPtr, keySize);

    memory_unmap_raw_allocation(&rawKeyKey, (void **)&rawKeyPtr);

    memcpy((void *)outRawKeyKeyPtr, &rawKeyKey, sizeof(struct memory_raw_allocation_key));

    return B32_TRUE;
}

static b32
_basic_dict_get_bucket_by_index(const struct memory_allocation_key *dictKeyPtr,
    u64 bucketIndex, const struct memory_allocation_key *outBucketKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(dictKeyPtr)))
    {
        return B32_FALSE;
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, 
        (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    if (bucketIndex >= dictPtr->tableBucketCount)
    {
        memory_unmap_alloc((void **)&dictPtr);
        memory_get_null_allocation_key(outBucketKeyPtr);

        return B32_FALSE;
    }

    const struct memory_allocation_key *tablePtr;
    const struct memory_allocation_key *bucketKeyPtr;
    {

        memory_error_code resultCode = memory_map_alloc(&dictPtr->bucketTableKey, 
        (void **)&tablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictPtr);

            memory_get_null_allocation_key(outBucketKeyPtr);

            return B32_FALSE;
        }

        bucketKeyPtr = &tablePtr[bucketIndex];
    }

    memcpy((void *)outBucketKeyPtr, bucketKeyPtr, sizeof(struct memory_allocation_key));

    memory_unmap_alloc((void **)&tablePtr);
    memory_unmap_alloc((void **)&dictPtr);

    return B32_TRUE;
}

static b32
_basic_dict_get_node_by_key(const struct memory_allocation_key *dictKeyPtr,
    void *keyPtr, const struct memory_allocation_key *outNodeKeyPtr,
    const struct memory_allocation_key *outBucketKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(dictKeyPtr)))
    {
        if (outNodeKeyPtr)
        {
            memory_get_null_allocation_key(outNodeKeyPtr);
        }

        return B32_FALSE;
    }

    if (!keyPtr)
    {
        if (outNodeKeyPtr)
        {
            memory_get_null_allocation_key(outNodeKeyPtr);
        }

        if (outBucketKeyPtr)
        {
            memory_get_null_allocation_key(outBucketKeyPtr);
        }

        return B32_FALSE;
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, 
        (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_get_null_allocation_key(outNodeKeyPtr);

            return B32_FALSE;
        }
    }

    struct utils_string_hash dictHash;

    if (!(dictPtr->hashFunc(dictPtr, keyPtr, &dictHash)))
    {
        memory_unmap_alloc((void **)&dictPtr);
        memory_get_null_allocation_key(outNodeKeyPtr);

        return B32_FALSE;
    }

    const struct memory_allocation_key *bucketTablePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->bucketTableKey, 
        (void **)&bucketTablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictPtr);
            memory_get_null_allocation_key(outNodeKeyPtr);

            return B32_FALSE;
        }
    }
    
    const struct memory_allocation_key *listKeyPtr = &bucketTablePtr[
        dictPtr->tableBucketCount%dictHash.hash];

    const struct memory_allocation_key headNodeKey;

    if (!(basic_list_get_next_node(listKeyPtr, NULL, &headNodeKey)))
    {
        memory_unmap_alloc((void **)&bucketTablePtr);
        memory_unmap_alloc((void **)&dictPtr);
        memory_get_null_allocation_key(outNodeKeyPtr);

        return B32_FALSE;
    }

    const struct memory_allocation_key currentNodeKey;

    memcpy((void *)&currentNodeKey, &headNodeKey, sizeof(struct memory_allocation_key));

    do 
    {
        struct basic_dict_node *currentNodePtr;

        if (!(basic_list_map_data(&currentNodeKey, (void **)&currentNodePtr)))
        {
            memory_unmap_alloc((void **)&bucketTablePtr);
            memory_unmap_alloc((void **)&dictPtr);
            memory_get_null_allocation_key(outNodeKeyPtr);

            return B32_FALSE;
        }

        void *lhsKeyPtr;

        memory_error_code resultCode = memory_map_raw_allocation(&currentNodePtr->rawKeyKey, &lhsKeyPtr);

        if (resultCode != MEMORY_OK)
        {
            basic_list_unmap_data(listKeyPtr, &currentNodeKey, (void **)&currentNodePtr);

            const struct memory_allocation_key nextNodeKey;

            if (!(basic_list_get_next_node(listKeyPtr, &currentNodeKey, &nextNodeKey)))
            {
                basic_list_unmap_data(listKeyPtr, &currentNodeKey, (void **)&currentNodePtr);
                memory_unmap_alloc((void **)&bucketTablePtr);
                memory_unmap_alloc((void **)&dictPtr);
                memory_get_null_allocation_key(outNodeKeyPtr);

                return B32_FALSE;
            }

            if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&headNodeKey, &nextNodeKey)))
            {
                memcpy((void *)&currentNodeKey, &nextNodeKey, sizeof(struct memory_allocation_key));

                continue;
            }
            else 
            {
                memory_get_null_allocation_key(&currentNodeKey);

                break;
            }
        }

        if (!(memcmp(lhsKeyPtr, keyPtr, dictPtr->keyByteSize)))
        {
            memory_unmap_raw_allocation(&currentNodePtr->rawKeyKey, &lhsKeyPtr);
            basic_list_unmap_data(listKeyPtr, &currentNodeKey, (void **)&currentNodePtr);
            memory_unmap_alloc((void **)&bucketTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            if (outNodeKeyPtr)
            {
                memcpy((void *)outNodeKeyPtr, &currentNodeKey, sizeof(struct memory_allocation_key));
            }

            if (outBucketKeyPtr)
            {
                memcpy((void *)outBucketKeyPtr, listKeyPtr, sizeof(struct memory_allocation_key));
            }

            return B32_TRUE;
        }
        
        memory_unmap_raw_allocation(&currentNodePtr->rawKeyKey, &lhsKeyPtr);

        const struct memory_allocation_key nextNodeKey;

        if (!(basic_list_get_next_node(listKeyPtr, &currentNodeKey, &nextNodeKey)))
        {
            basic_list_unmap_data(listKeyPtr, &currentNodeKey, (void **)&currentNodePtr);
            memory_unmap_alloc((void **)&bucketTablePtr);
            memory_unmap_alloc((void **)&dictPtr);
            memory_get_null_allocation_key(outNodeKeyPtr);

            return B32_FALSE;
        }
        
        basic_list_unmap_data(listKeyPtr, &currentNodeKey, (void **)&currentNodePtr);

        if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&headNodeKey, &nextNodeKey)))
        {
            memcpy((void *)&currentNodeKey, &nextNodeKey, sizeof(struct memory_allocation_key));
        }
        else 
        {
            memory_get_null_allocation_key(&currentNodeKey);
        }
    } while (!(MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)));

    memory_unmap_alloc((void **)&bucketTablePtr);
    memory_unmap_alloc((void **)&dictPtr);

    return B32_FALSE;
}

static b32
_basic_dict_clear_bucket(const struct memory_allocation_key *dictKeyPtr,
    u64 bucketIndex)
{
    const struct memory_allocation_key bucketKey;

    if (!(_basic_dict_get_bucket_by_index(dictKeyPtr, bucketIndex, &bucketKey)))
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key headNodeKey;

    if (!(basic_list_get_next_node(&bucketKey, NULL, &headNodeKey)))
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

    const struct memory_allocation_key currentNodeKey;

    memcpy((void *)&currentNodeKey, &headNodeKey, sizeof(struct memory_allocation_key));

    do 
    {
        const struct memory_allocation_key nextNodeKey;

        if (!(basic_list_get_next_node(&bucketKey, &currentNodeKey, &nextNodeKey)))
        {
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        if (!(basic_list_move_node(&bucketKey, &dictPtr->freeNodeList, 
        &currentNodeKey)))
        {
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
    } while (!(MEMORY_IS_ALLOCATION_NULL(&currentNodeKey)));

    memory_unmap_alloc((void **)&dictPtr);

    return B32_TRUE;
}

b32
basic_dict_create(const struct memory_page_key *memoryPageKeyPtr, basic_dict_hash_func hashFunc,
    basic_dict_create_key_copy_func keyCopyFunc, u32 initBucketCount, u64 *keySize, 
    const struct memory_allocation_key *userPtrKeyPtr, const struct memory_allocation_key *outDictKeyPtr)
{
    if ((MEMORY_IS_PAGE_NULL(memoryPageKeyPtr)))
    {
        if (outDictKeyPtr)
        {
            memory_get_null_allocation_key(outDictKeyPtr);
        }

        return B32_FALSE;
    }

    if (!outDictKeyPtr)
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

    basic_list_create(memoryPageKeyPtr, &dictPtr->freeNodeList);

    if (keyCopyFunc)
    {
        dictPtr->keyCopyFunc = keyCopyFunc;
        dictPtr->keyByteSize = *keySize;
    }
    else 
    {
        dictPtr->keyCopyFunc = &_default_key_copy_func;
        dictPtr->keyByteSize = sizeof(const char *);
    }

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

    const struct memory_allocation_key foundNodeBucketKey;
    const struct memory_allocation_key foundNodeKey;

    if (!(_basic_dict_get_node_by_key(dictKeyPtr, keyPtr, &foundNodeKey, 
    &foundNodeBucketKey)))
    {
        return B32_FALSE;
    }

    struct basic_dict_node *nodePtr;

    if (!(basic_list_map_data(&foundNodeKey, (void **)&nodePtr)))
    {
        *outDataPtr = NULL;

        return B32_FALSE;
    }

    u8 *resultPtr;

    memory_error_code resultCode = memory_map_alloc(&nodePtr->dataKey, 
    (void **)&resultPtr);

    if (resultCode != MEMORY_OK)
    {
        basic_list_unmap_data(&foundNodeBucketKey, &foundNodeKey, 
        (void **)&nodePtr);

        return B32_FALSE;
    }

    resultPtr += nodePtr->dataByteOffset;

    *outDataPtr = resultPtr;

    basic_list_unmap_data(&foundNodeBucketKey, &foundNodeKey, 
    (void **)&nodePtr);

    return B32_TRUE;
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

    const struct memory_allocation_key foundBucketKey;
    const struct memory_allocation_key foundNodeKey;
    
    if (!(_basic_dict_get_node_by_key(dictKeyPtr, keyPtr, &foundNodeKey, 
        &foundBucketKey)))
    {
        return B32_FALSE;
    }

    struct basic_dict_node *nodePtr;

    if (!(basic_list_map_data(&foundNodeKey, (void **)&nodePtr)))
    {
        return B32_FALSE;
    }

    u8 *resultPtr = *outDataPtr;
    resultPtr -= nodePtr->dataByteOffset;

    memory_unmap_alloc((void **)resultPtr);

    *outDataPtr = NULL;

    basic_list_unmap_data(&foundBucketKey, &foundNodeKey, 
    (void **)&nodePtr);

    return B32_TRUE;
}

b32
basic_dict_push_data(const struct memory_allocation_key *dictKeyPtr,
    void *keyPtr, p64 *dataByteOffset, u64 *dataByteSize,
    const struct memory_allocation_key *dataKeyPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(dictKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!keyPtr)
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key foundNodeKey;
    const struct memory_allocation_key foundBucketKey;

    if (!(_basic_dict_get_node_by_key(dictKeyPtr, keyPtr, &foundNodeKey,
            &foundBucketKey)))
    {
        struct basic_dict *dictPtr;

        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, 
        (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        struct utils_string_hash dictHash;

        if (!(dictPtr->hashFunc(dictPtr, keyPtr, &dictHash)))
        {
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        const struct memory_allocation_key *bucketTablePtr;

        resultCode = memory_map_alloc(&dictPtr->bucketTableKey, (void **)&bucketTablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        const struct memory_allocation_key *bucketKeyPtr = &bucketTablePtr[dictPtr->tableBucketCount%
            dictHash.hash];

        if (!(basic_list_get_next_node(&dictPtr->freeNodeList, 
        NULL, &foundNodeKey)))
        {
            const struct memory_allocation_key dictNodeKey;

            resultCode = memory_alloc(&dictPtr->pageKey, sizeof(struct basic_dict_node), 
            NULL, &dictNodeKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&bucketTablePtr);
                memory_unmap_alloc((void **)&dictPtr);

                return B32_FALSE;
            }

            if (!(basic_list_append(bucketKeyPtr, 0, 
            sizeof(struct basic_dict_node), &dictNodeKey, 
            &foundNodeKey)))
            {
                memory_free(&dictNodeKey);
                memory_unmap_alloc((void **)&bucketTablePtr);
                memory_unmap_alloc((void **)&dictPtr);

                return B32_FALSE;
            }
        }
        else 
        {
            if (!(basic_list_move_node(&dictPtr->freeNodeList, 
            bucketKeyPtr, &foundNodeKey)))
            {
                memory_unmap_alloc((void **)&bucketTablePtr);
                memory_unmap_alloc((void **)&dictPtr);

                return B32_FALSE;
            }
        }

        memcpy((void *)&foundBucketKey, bucketKeyPtr, sizeof(struct memory_allocation_key));

        memory_unmap_alloc((void **)&bucketTablePtr);
        memory_unmap_alloc((void **)&dictPtr);
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            basic_list_free_node(&foundBucketKey, &foundNodeKey);

            return B32_FALSE;
        }
    }

    struct basic_dict_node *nodePtr;
    const struct memory_raw_allocation_key rawKeyKey;

    if (!(dictPtr->keyCopyFunc(dictPtr, keyPtr, &rawKeyKey)))
    {
        basic_list_free_node(&foundBucketKey, &foundNodeKey);

        return B32_FALSE;
    }

    memory_unmap_alloc((void **)&dictPtr);

    if (!(basic_list_map_data(&foundNodeKey, (void **)&nodePtr)))
    {
        memory_raw_free(&rawKeyKey);
        basic_list_free_node(&foundBucketKey, &foundNodeKey);

        // TODO:

        return B32_FALSE;
    }

    memcpy((void *)&nodePtr->rawKeyKey, &rawKeyKey, sizeof(struct memory_raw_allocation_key));

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
    else 
    {
        memory_get_null_allocation_key(&nodePtr->dataKey);
    }

    basic_list_unmap_data(&foundBucketKey, &foundNodeKey, (void **)&nodePtr);

    return B32_TRUE;
}

b32
basic_dict_get_data(const struct memory_allocation_key *dictKeyPtr,
    void *keyPtr, p64 *dataByteOffset, u64 *dataByteSize,
    const struct memory_allocation_key *outDataKeyPtr)
{
    const struct memory_allocation_key foundBucketKey;
    const struct memory_allocation_key foundNodeKey;

    if (!(_basic_dict_get_node_by_key(dictKeyPtr, keyPtr, &foundNodeKey, 
    &foundBucketKey)))
    {
        return B32_FALSE;
    }

    struct basic_dict_node *nodePtr;

    if (!(basic_list_map_data(&foundNodeKey, (void **)&nodePtr)))
    {
        return B32_FALSE;
    }

    if (dataByteOffset)
    {
        *dataByteOffset = nodePtr->dataByteOffset;
    }

    if (dataByteSize)
    {
        *dataByteSize = nodePtr->dataByteSize;
    }

    if (outDataKeyPtr)
    {
        memcpy((void *)outDataKeyPtr, &nodePtr->dataKey, sizeof(struct memory_allocation_key));
    }

    basic_list_unmap_data(&foundBucketKey, &foundNodeKey, (void **)&nodePtr);

    return B32_TRUE;
}

b32
basic_dict_remove(const struct memory_allocation_key *dictKeyPtr, void *keyPtr)
{
    const struct memory_allocation_key foundBucketKey;
    const struct memory_allocation_key foundNodeKey;

    if (!(_basic_dict_get_node_by_key(dictKeyPtr, keyPtr, &foundNodeKey, 
    &foundBucketKey)))
    {
        return B32_FALSE;
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, 
        (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    b32 isResult = basic_list_move_node(&foundBucketKey, 
    &dictPtr->freeNodeList, &foundNodeKey);

    memory_unmap_alloc((void **)&dictPtr);

    return isResult;
}

b32
basic_dict_clear(const struct memory_allocation_key *dictKeyPtr)
{
    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, 
        (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    u32 bucketCount = dictPtr->tableBucketCount;

    memory_unmap_alloc((void **)&dictPtr);

    for (u32 bucketIndex = 0; bucketIndex < bucketCount; ++bucketIndex)
    {
        _basic_dict_clear_bucket(dictKeyPtr, bucketIndex);
    }

    return B32_TRUE;
}

b32
basic_dict_get_is_found(const struct memory_allocation_key *dictKeyPtr, void *keyPtr)
{
    return _basic_dict_get_node_by_key(dictKeyPtr, keyPtr, NULL, NULL);
}

b32
basic_dict_destroy(const struct memory_allocation_key *dictKeyPtr)
{
    struct basic_dict *dictPtr;
    const struct memory_allocation_key *bucketTableKeyPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, 
        (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&dictPtr->bucketTableKey, 
        (void **)&bucketTableKeyPtr);
    }

    memory_unmap_alloc((void **)&dictPtr);

    for (u32 bucketIndex = 0; bucketIndex < dictPtr->tableBucketCount; ++bucketIndex)
    {
        const struct memory_allocation_key bucketKey;

        if ((_basic_dict_get_bucket_by_index(dictKeyPtr, bucketIndex, &bucketKey)))
        {
            basic_list_destroy(&bucketKey);
        }
    }

    memory_map_alloc(dictKeyPtr, (void **)&dictPtr);
    basic_list_destroy(&dictPtr->freeNodeList);
    memory_free(&dictPtr->bucketTableKey);
    memory_unmap_alloc((void **)&dictPtr);
    memory_free(dictKeyPtr);

    return B32_TRUE;
}