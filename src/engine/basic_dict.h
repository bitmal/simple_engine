#ifndef __BASIC_DICT_H
#define __BASIC_DICT_H

#include "memory.h"
#include "types.h"

#include <stdio.h>

typedef u32 basic_dict_id;

struct utils_string_hash;

struct basic_dict;

typedef b32 (*basic_dict_hash_func)(const struct memory_allocation_key *dictKeyPtr, void *key, 
    struct utils_string_hash *outHashPtr);
typedef b32 (*basic_dict_data_is_found_equal_func)(struct basic_dict *dict, void *key);

struct basic_dict_pair;
struct basic_dict;

#define BASIC_DICT_NULL_PTR ((p64)0)

b32
basic_dict_create(const struct memory_page_key *memoryPageKeyPtr, basic_dict_hash_func hashFunc, 
    u32 initBucketCount, u64 keySize, const struct memory_allocation_key *userPtrKeyPtr, 
    const struct memory_allocation_key *databaseKeyPtr);

b32
basic_dict_get(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    const struct memory_allocation_key *outDataKeyPtr);

b32
basic_dict_set(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    const struct memory_allocation_key *dataKeyPtr);

b32
basic_dict_remove(const struct memory_allocation_key *dictKeyPtr, void *keyPtr);

b32
basic_dict_clear(const struct memory_allocation_key *dictKeyPtr);

b32
basic_dict_get_is_found(const struct memory_allocation_key *dictKeyPtr, void *keyPtr);

#endif