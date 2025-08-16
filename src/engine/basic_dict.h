#ifndef __BASIC_DICT_H
#define __BASIC_DICT_H

#include "memory.h"
#include "types.h"

#include <stdio.h>

typedef u32 basic_dict_id;

struct utils_string_hash;

struct basic_dict;

typedef b32 (*basic_dict_hash_func)(struct basic_dict *dictPtr, void *key, 
    struct utils_string_hash *outHashPtr);
typedef b32 (*basic_dict_data_is_found_equal_func)(struct basic_dict *dict, void *key);

#define BASIC_DICT_NULL_PTR ((p64)0)

b32
basic_dict_create(const struct memory_page_key *memoryPageKeyPtr, basic_dict_hash_func hashFunc, 
    u32 initBucketCount, u64 keySize, const struct memory_allocation_key *userPtrKeyPtr,
    p64 databaseByteOffset, u64 databaseByteSize,
    const struct memory_allocation_key *databaseKeyPtr,
    const struct memory_allocation_key *outDictKeyPtr);

b32
basic_dict_map_data(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    void **outDataPtr);

b32
basic_dict_unmap_data(const struct memory_allocation_key *dictKeyPtr, void *keyPtr,
    void **outDataPtr);

b32
basic_dict_set_data_info(const struct memory_allocation_key *dictKeyPtr,
    void *keyPtr, p64 *databaseByteoffset, u64 *databaseByteSize);

b32
basic_dict_remove(const struct memory_allocation_key *dictKeyPtr, void *keyPtr);

b32
basic_dict_clear(const struct memory_allocation_key *dictKeyPtr);

b32
basic_dict_reset(const struct memory_allocation_key *dictKeyPtr, const struct memory_allocation_key *databaseKeyPtr);

b32
basic_dict_get_is_found(const struct memory_allocation_key *dictKeyPtr, void *keyPtr);

#endif