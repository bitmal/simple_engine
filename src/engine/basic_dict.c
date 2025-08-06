#include "basic_dict.h"
#include "types.h"
#include "utils.h"
#include "basic_list.h"
#include "memory.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct basic_dict_pair
{
    const void *key;
    u64 hash;
    p64 databaseByteOffset;
};

struct basic_dict
{
    const struct memory_page_key pageKey;
    const struct memory_allocation_key tableKey;
    const struct memory_allocation_key userPtrKey;
    const struct memory_allocation_key freePairKeyList;
    u64 freePairListNodeCount;
    u32 tableBucketCount;
    basic_dict_hash_func hashFunc;
    u64 keyByteSize;
};

static b32 
_default_hash_func(const struct memory_allocation_key *dictKeyPtr, void *key, 
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
_basic_dict_remove_node(const struct memory_allocation_key *dictKeyPtr, void *keyPtr)
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

        struct basic_dict_pair *pairPtr;
        
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
        const struct memory_allocation_key *pairKeyPtr;

        if (!(basic_list_get_data(&currentPairNodeKey, pairKeyPtr)))
        {
            memory_unmap_alloc((void **)&tablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        struct basic_dict_pair *pairPtr;

        memory_error_code resultCode = memory_map_alloc(&currentPairNodeKey, 
        (void **)&pairPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&tablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }

        if (listNodeCount > 1)
        {
        }

        memory_unmap_alloc((void **)&pairPtr);
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
    const struct memory_allocation_key *databaseKeyPtr)
{
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
        &dictPtr->tableKey);

    memory_set_alloc_offset_width(&dictPtr->tableKey, 0, 
        sizeof(struct memory_allocation_key)*initBucketCount, '\0');

    dictPtr->hashFunc = hashFunc ? hashFunc : &_default_hash_func;
    memcpy((void *)&dictPtr->databaseKey, databaseKeyPtr, sizeof(struct memory_allocation_key));

    memcpy((struct memory_allocation_key*)&dictPtr->userPtrKey, userPtrKeyPtr, sizeof(struct memory_allocation_key));

    dictPtr->freePairListNodeCount = 0;

    memory_unmap_alloc((void **)&dictPtr);

    return B32_TRUE;
}

b32
basic_dict_get(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    const struct memory_allocation_key *outDataKeyPtr)
{
    if (!dictKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): 'dictKeyPtr' argument is NULL. Cannot get data allocation.",
            __func__, __LINE__);

        return B32_FALSE;
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): 'dictKeyPtr' argument is NULL. Cannot get data allocation.",
                __func__, __LINE__);

            return B32_FALSE;
        }
    }

    struct utils_string_hash hash;
    dictPtr->hashFunc(dictKeyPtr, keyPtr, &hash);

    struct memory_allocation_key *dictTablePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->tableKey, (void **)&dictTablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictPtr);
            
            utils_fprintfln(stderr, "%s(Line: %d): Failure to memory map basic dict table.",
                __func__, __LINE__);

            return B32_FALSE;
        }
    }

    void *databasePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->databaseKey, (void **)&databasePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            utils_fprintfln(stderr, "%s(Line: %d): Failure to memory map database.",
                __func__, __LINE__);

            return B32_FALSE;
        }
    }

    struct memory_allocation_key *listKeyPtr = &dictTablePtr[dictPtr->tableBucketCount%hash.hash];
    struct memory_allocation_key headPairListNodeKey;
    struct memory_allocation_key pairListNodeKey;

    if (!(basic_list_get_next_node(listKeyPtr, NULL, &headPairListNodeKey)))
    {
        memory_unmap_alloc(&databasePtr);
        memory_unmap_alloc((void **)&dictTablePtr);
        memory_unmap_alloc((void **)&dictPtr);

        return B32_FALSE;
    }

    memcpy(&pairListNodeKey, &headPairListNodeKey, sizeof(struct memory_allocation_key));

    do
    {
        const struct memory_allocation_key pairKey;

        if (!(basic_list_get_data(&pairListNodeKey, &pairKey)))
        {
            memory_unmap_alloc((void **)&databasePtr);
            memory_unmap_alloc((void **)&dictTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            // TODO:

            return B32_FALSE;
        }

        struct basic_dict_pair *pairPtr;

        memory_error_code resultCode = memory_map_alloc(&pairKey, (void **)&pairPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&databasePtr);
            memory_unmap_alloc((void **)&dictTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            // TODO:

            return B32_FALSE;
        }

        if (!(strcmp(keyPtr, pairPtr->key)))
        {
            memcpy((void *)outDataKeyPtr, (void *)(
                (p64)databasePtr + pairPtr->databaseByteOffset), 
                sizeof(struct memory_allocation_key));

            memory_get_null_allocation_key(&pairListNodeKey);
        }

        memory_unmap_alloc((void **)&pairPtr);

        if (!(MEMORY_IS_ALLOCATION_NULL((&pairListNodeKey))))
        {
            const struct memory_allocation_key nextListNodeKey;

            basic_list_get_next_node(listKeyPtr, &pairListNodeKey, 
            &nextListNodeKey);

            if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&nextListNodeKey, &headPairListNodeKey)))
            {
                memcpy((void *)&pairListNodeKey, &nextListNodeKey, 
                sizeof(struct memory_allocation_key));
            }
            else 
            {
                memory_get_null_allocation_key(&pairListNodeKey);
            }
        }
    } while (!(MEMORY_IS_ALLOCATION_NULL((&pairListNodeKey))));

    memory_unmap_alloc((void **)&dictTablePtr);
    memory_unmap_alloc((void **)&dictPtr);
    memory_unmap_alloc((void **)&databasePtr);

    return B32_TRUE;
}

b32
basic_dict_set(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    const struct memory_allocation_key *dataKeyPtr)
{
    if (!dictKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): 'dictKeyPtr' argument is NULL. Cannot get data allocation.",
            __func__, __LINE__);

        return B32_FALSE;
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): 'dictKeyPtr' argument is NULL. Cannot get data allocation.",
                __func__, __LINE__);

            return B32_FALSE;
        }
    }

    struct utils_string_hash hash;
    dictPtr->hashFunc(dictKeyPtr, keyPtr, &hash);

    struct memory_allocation_key *dictTablePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->tableKey, 
        (void **)&dictTablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictPtr);
            
            utils_fprintfln(stderr, "%s(Line: %d): Failure to memory map basic dict table.",
                __func__, __LINE__);

            return B32_FALSE;
        }
    }

    void *databasePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->databaseKey, (void **)&databasePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&dictTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            utils_fprintfln(stderr, "%s(Line: %d): Failure to memory map database.",
                __func__, __LINE__);

            return B32_FALSE;
        }
    }

    struct memory_allocation_key *listKeyPtr = &dictTablePtr[dictPtr->tableBucketCount%hash.hash];
    struct memory_allocation_key headPairListNodeKey;
    struct memory_allocation_key pairListNodeKey;

    if (!(basic_list_get_next_node(listKeyPtr, NULL, &headPairListNodeKey)))
    {
        memory_unmap_alloc(&databasePtr);
        memory_unmap_alloc((void **)&dictTablePtr);
        memory_unmap_alloc((void **)&dictPtr);

        return B32_FALSE;
    }
    
    memcpy(&pairListNodeKey, &headPairListNodeKey, sizeof(struct memory_allocation_key));

    do
    {
        const struct memory_allocation_key pairKey;

        if (!(basic_list_get_data(&pairListNodeKey, &pairKey)))
        {
            memory_unmap_alloc((void **)&databasePtr);
            memory_unmap_alloc((void **)&dictTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            // TODO:

            return B32_FALSE;
        }

        struct basic_dict_pair *pairPtr;

        memory_error_code resultCode = memory_map_alloc(&pairKey, (void **)&pairPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&databasePtr);
            memory_unmap_alloc((void **)&dictTablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            // TODO:

            return B32_FALSE;
        }

        if (!(strcmp(keyPtr, pairPtr->key)))
        {
            memcpy((void *)((p64)databasePtr + pairPtr->databaseByteOffset), 
            dataKeyPtr, sizeof(struct memory_allocation_key));

            memory_get_null_allocation_key(&pairListNodeKey);
        }

        memory_unmap_alloc((void **)&pairPtr);

        if (!(MEMORY_IS_ALLOCATION_NULL((&pairListNodeKey))))
        {
            const struct memory_allocation_key nextListNodeKey;

            basic_list_get_next_node(listKeyPtr, &pairListNodeKey, 
            &nextListNodeKey);

            if (!(MEMORY_IS_ALLOCATION_KEY_EQUAL(&nextListNodeKey, &headPairListNodeKey)))
            {
                memcpy((void *)&pairListNodeKey, &nextListNodeKey, 
                sizeof(struct memory_allocation_key));
            }
            else 
            {
                memory_get_null_allocation_key(&pairListNodeKey);
            }
        }
    } while (!(MEMORY_IS_ALLOCATION_NULL((&pairListNodeKey))));

    memory_unmap_alloc((void **)&dictTablePtr);
    memory_unmap_alloc((void **)&dictPtr);
    memory_unmap_alloc((void **)&databasePtr);

    return B32_TRUE;
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