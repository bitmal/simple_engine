#include "basic_dict.h"
#include "utils.h"
#include "memory.h"

#include <stdlib.h>
#include <assert.h>

struct basic_dict_pair
{
    const void *key;
    u64 hash;
    basic_dict_rel_ptr dataPtr;
    struct basic_dict_pair *next;
};

struct basic_dict
{
    const struct memory_page_key *
    const struct memory_allocation_key tableKey;
    const struct memory_allocation_key userPtrKey;
    u32 tableBucketCount;
    basic_dict_hash_func hashFunc;
};

static b32 
_default_hash_func(const struct memory_allocation_key *dictKeyPtr, void *key)
{
    struct utils_string_hash resultHash;

    if (!(utils_generate_hash_from_string(key, &resultHash)))
    {
        return B32_FALSE;
    }

    basic_dict_get_is_found(const struct memory_allocation_key *memoryKeyPtr, void *key)

    return B32_TRUE;
}

b32
basic_dict_create(const struct memory_page_key *memoryPageKeyPtr, basic_dict_hash_func hashFunc, 
    u32 initBucketCount, u64 keySize, const struct memory_allocation_key *userPtr, 
    const struct memory_allocation_key *database)
{
    const struct memory_allocation_key dictKey;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_alloc(memoryPageKeyPtr, 
            sizeof(struct basic_dict) + sizeof(struct memory_raw_allocation_key), &dictKey) != MEMORY_OK))
        {
            fprintf(stderr, "%s(%d): Memory Error(%u), cannot allocate basic_dict.\n",
                    __FUNCTION__, __LINE__, (u32)resultCode);

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

            fprintf(stderr, "%s(%d): Memory Error(%u), cannot map basic_dict allocation.\n",
                    __FUNCTION__, __LINE__, (u32)resultCode);

            return B32_FALSE;
        }
    }

    dictPtr->tableBucketCount = initBuckets;
    dictPtr->__table = memory_alloc(&dictKey, sizeof(struct basic_dict_pair *)*initBuckets);
    memset(dict->__table, 0, sizeof(struct basic_dict_pair *)*initBuckets);
    dict->__hashFunc = hashFunc ? hashFunc : &_default_hash_func;
    dict->userPtr = userPtr;

    return B32_TRUE;
}

void *
basic_dict_get(struct basic_dict *dict, void *database, void *key)
{
    const u64 hash = dict->__hashFunc(dict, key);
    struct basic_dict_pair *pair = dict->__table[hash%dict->buckets];

    while (pair)
    {
        if (pair->hash == hash)
        {
            if (pair->dataPtr != BASIC_DICT_NULL_REL_PTR)
            {
                return (void *)((basic_dict_rel_ptr)database + pair->dataPtr);
            }
            else
            {
                return NULL;
            }
        }

        pair = pair->next;
    }

    return NULL;
}

b32
basic_dict_set(struct basic_dict *dict, struct memory *mem, void *database, void *key, size_t keySize, void *data)
{
    const u64 hash = dict->__hashFunc(dict, key);
    const basic_dict_id index = hash%dict->buckets;

    b32 result = B32_FALSE;

    if (dict->__table[index])
    {
        struct basic_dict_pair *pair = dict->__table[index];
        b32 isFound = B32_FALSE;

        while (pair && pair->next)
        {
            if (pair->hash == hash)
            {
                isFound = B32_TRUE;
                break;
            }

            pair = pair->next;
        }

        if (isFound)
        {
            if (data)
            {
                pair->dataPtr = (basic_dict_rel_ptr)data - (basic_dict_rel_ptr)database;
            }
            else
            {
                pair->dataPtr = BASIC_DICT_NULL_REL_PTR;
            }
        }
        else
        {
            pair->next = memory_alloc(mem, sizeof(struct basic_dict_pair));
            pair->next->key = memory_alloc(mem, keySize);
            memcpy((void *)pair->next->key, key, keySize);
            pair->next->hash = hash;
            pair->next->next = NULL;
            
            if (data)
            {
                pair->next->dataPtr = (basic_dict_rel_ptr)data - (basic_dict_rel_ptr)database;
            }
            else
            {
                pair->next->dataPtr = BASIC_DICT_NULL_REL_PTR;
            }
        }

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