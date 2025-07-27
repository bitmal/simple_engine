#ifndef __BASIC_DICT_H
#define __BASIC_DICT_H

#include "memory.h"
#include "types.h"

#include <stdio.h>

typedef i32 basic_dict_id;
typedef u64 basic_dict_rel_ptr;

struct basic_dict;
typedef b32 (*basic_dict_hash_func)(const struct memory_allocation_key *dictKeyPtr, void *key);
typedef b32 (*basic_dict_data_is_found_equal_func)(struct basic_dict *dict, void *key);

#define BASIC_DICT_NULL_REL_PTR ((basic_dict_rel_ptr)0 - (basic_dict_rel_ptr)1)
#define BASIC_DICT_MAX_ADDRESS (BASIC_DICT_NULL_REL_PTR - 1)

#define BASIC_DICT_DEFAULT_BUCKET_COUNT 101

struct basic_dict_pair;
struct basic_dict;

b32
basic_dict_create(const struct memory_page_key *memoryPageKeyPtr, basic_dict_hash_func hashFunc, 
    u32 initBucketCount, u64 keySize, const struct memory_allocation_key *userPtr, 
    const struct memory_allocation_key *database);
    
#define DICTIONARY(memoryPageKeyPtr, hashFunc, keySize, userKeyPtr, dataKeyPtr) \
    basic_dict_create(memoryPageKeyPtr, hashFunc, \
    BASIC_DICT_DEFAULT_BUCKET_COUNT, keySize, userPtr, database)

b32
basic_dict_get(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    const struct memory_allocation_key *outDataKeyPtr);

b32
basic_dict_set(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    const struct memory_allocation_key *dataKeyPtr);

void
basic_dict_clear(const struct memory_allocation_key *dictKeyPtr);

b32
basic_dict_get_is_found(const struct memory_allocation_key *dictKeyPtr, void *keyPtr);

#endif