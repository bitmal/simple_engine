#include "basic_dict.h"
#include "utils.h"
#include "memory.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct basic_dict_pair
{
    const void *key;
    u64 hash;
    p64 databaseByteOffset;
    struct basic_dict_pair *next;
};

struct basic_dict
{
    const struct memory_page_key pageKey;
    const struct memory_allocation_key tableKey;
    const struct memory_allocation_key databaseKey;
    const struct memory_allocation_key userPtrKey;
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

b32
basic_dict_create(const struct memory_page_key *memoryPageKeyPtr, basic_dict_hash_func hashFunc, 
    u32 initBucketCount, u64 keySize, const struct memory_allocation_key *userPtrKeyPtr, 
    const struct memory_allocation_key *databaseKeyPtr)
{
    const struct memory_allocation_key dictKey;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_alloc(memoryPageKeyPtr, 
            sizeof(struct basic_dict) + sizeof(struct memory_raw_allocation_key), &dictKey) != MEMORY_OK))
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

    {
        memory_error_code resultCode = memory_alloc(memoryPageKeyPtr, 
            sizeof(struct basic_dict_pair *)*initBucketCount, &dictPtr->tableKey);

        if (resultCode != MEMORY_OK)
        {
            memory_free(&dictKey);
            
            utils_fprintfln(stderr, "%s(Line: %d): Cannot alloc basic_dict table.",
                    __func__, __LINE__);
        }
    }

    memory_set_alloc_offset_width(&dictPtr->tableKey, 0, 
        sizeof(struct basic_dict_pair *)*initBucketCount, '\0');

    dictPtr->hashFunc = hashFunc ? hashFunc : &_default_hash_func;
    memcpy((void *)&dictPtr->databaseKey, databaseKeyPtr, sizeof(struct memory_allocation_key));

    memcpy((struct memory_allocation_key *)&dictPtr->userPtrKey, userPtrKeyPtr, sizeof(struct memory_allocation_key));

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

    struct basic_dict_pair *dictTablePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->tableKey, (void **)&dictTablePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)dictPtr);
            
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
            utils_fprintfln(stderr, "%s(Line: %d): Failure to memory map database.",
                __func__, __LINE__);

            return B32_FALSE;
        }
    }

    struct basic_dict_pair *pair = &dictTablePtr[dictPtr->tableBucketCount%hash.hash];

    while (pair)
    {
        if (pair->hash == hash.hash)
        {
            memcpy((void *)outDataKeyPtr, (void *)((p64)databasePtr + pair->databaseByteOffset), 
                sizeof(struct memory_allocation_key));

            break;
        }

        pair = pair->next;
    }

    memory_unmap_alloc((void **)&dictTablePtr);
    memory_unmap_alloc((void **)&dictPtr);
    memory_unmap_alloc((void **)&databasePtr);

    return pair ? B32_TRUE : B32_FALSE;
}

b32
basic_dict_set(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    const struct memory_allocation_key *dataKeyPtr)
{
    if (!dictKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): 'dictKeyPtr' argument cannot be NULL."
            "Aborting.");

        return B32_FALSE;
    }

    if (!keyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): 'keyPtr' argument cannot be NULL."
            "Aborting.");

        return B32_FALSE;
    }

    if (!dataKeyPtr)
    {
        utils_fprintfln(stderr, "%s(Line: %d): 'dataKeyPtr' argument cannot be NULL."
            "Aborting.");

        return B32_FALSE;
    }

    struct basic_dict *dictPtr;
    {
        memory_error_code resultCode = memory_map_alloc(dictKeyPtr, 
            (void **)&dictPtr);

        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): Could not memory map"
                "dict. Aborting.");

            return B32_FALSE;
        }
    }

    struct utils_string_hash hashResult;
    
    dictPtr->hashFunc(dictKeyPtr, keyPtr, &hashResult);
    u32 tableIndex = dictPtr->tableBucketCount%hashResult.hash;

    struct basic_dict_pair **tablePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->tableKey, 
            (void **)&tablePtr);
        
        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): Could not memory map" 
                "dict table. Aborting.");

            return B32_FALSE;
        }
    }
    
    void *databasePtr;
    {
        memory_error_code resultCode = memory_map_alloc(&dictPtr->databaseKey, 
            (void **)&databasePtr);
        
        if (resultCode != MEMORY_OK)
        {
            utils_fprintfln(stderr, "%s(Line: %d): Could not memory map" 
                "database. Aborting.");

            memory_unmap_alloc((void **)&tablePtr);
            memory_unmap_alloc((void **)&dictPtr);

            return B32_FALSE;
        }
    }

    b32 result = B32_FALSE;

    if (tablePtr[tableIndex])
    {
        struct basic_dict_pair *pair = tablePtr[tableIndex];
        b32 isFound = B32_FALSE;

        while (pair && pair->next)
        {
            if (pair->hash == hashResult.hash)
            {
                isFound = B32_TRUE;
                break;
            }

            pair = pair->next;
        }
        
        u8 *databaseBytePtr = (u8 *)databasePtr;

        if (isFound)
        {
        }
        else
        {
            void *dataPtr;
            {
                memory_error_code resultCode;

                if (resultCode != MEMORY_OK)
                {
                    utils_fprintfln(stderr, "%s(Line: %d): Could not memory map" 
                        "data. Aborting.");

                    return B32_FALSE;
                }
            }

            pair->next = memory_alloc(&dictPtr->pageKey, sizeof(struct basic_dict_pair));
            pair->next->key = memory_alloc(&dictPtr->pageKey, dictPtr->keyByteSize, &pair->next->key);

            memcpy((void *)pair->next->key, keyPtr, dictPtr->keyByteSize);

            pair->next->hash = hashResult.hash;
            pair->next->next = NULL;
            
            pair->next->databaseByteOffset = (p64)dataPtr - (p64)databasePtr;
        }

        //pair->next->

        result = B32_TRUE;
    }
    else
    {
        dict->__table[index] = memory_alloc(mem, sizeof(struct basic_dict_pair));
        dict->__table[index]->key = memory_alloc(mem, keySize);
        memcpy((void *)dict->__table[index]->key, key, keySize);
        dict->__table[index]->hash = hash;
        dict->__table[index]->next = NULL;

        if (data)
        {
            dict->__table[index]->dataPtr = (basic_dict_rel_ptr)data - (basic_dict_rel_ptr)database;
        }
        else
        {
            dict->__table[index]->dataPtr = BASIC_DICT_NULL_REL_PTR;
        }

        result = B32_TRUE;
    }

    return result;
}

void
basic_dict_clear(struct basic_dict *dict)
{
    memset(dict->__table, 0, sizeof(struct basic_dict_pair *)*dict->buckets);
}

b32
basic_dict_get_is_found(const struct memory_allocation_key *dictKeyPtr, void *keyPtr)
{
    struct basic_dict *dictPtr;
    {
        memory_error_code
    }
    const u64 hash = dict->__hashFunc(dict, key);


    return B32_FALSE;
}