#ifndef __BASIC_LIST_H
#define __BASIC_LIST_H

#include "types.h"
#include "memory.h"

typedef u64 basic_list_id;
#define BASIC_LIST_NULL_ID ((basic_list_id)0)

b32
basic_list_create(const struct memory_page_key *memoryPageKeyPtr, 
    u64 databaseByteOffset,
    u64 databaseByteSize,
    const struct memory_allocation_key *databaseByteArrKeyPtr,
    const struct memory_allocation_key *outListKeyPtr);

b32
basic_list_get_node_id(const struct memory_allocation_key *nodeKeyPtr, 
    basic_list_id *outIdPtr);

b32
basic_list_get_node_by_id(const struct memory_allocation_key *listKeyPtr, 
    basic_list_id nodeId, 
    const struct memory_allocation_key *outNodekeyPtr);

b32
basic_list_get_is_node_active(const struct memory_allocation_key *nodeKeyPtr);

b32
basic_list_move_node(const struct memory_allocation_key *lhsListKeyPtr, 
    const struct memory_allocation_key *rhsListKeyPtr,
    const struct memory_allocation_key *nodeKeyPtr);

b32
basic_list_move_free_node(const struct memory_allocation_key *lhsListKeyPtr,
    const struct memory_allocation_key *rhsListKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
basic_list_get_active_count(const struct memory_allocation_key *listKeyPtr, 
    u64 *outActiveCount);

b32
basic_list_get_next_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outNodeKeyPtr);

b32
basic_list_get_prev_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outNodeKeyPtr);

b32
basic_list_map_data(const struct memory_allocation_key *nodeKeyPtr, void **outDataPtr);

b32
basic_list_unmap_data(const struct memory_allocation_key *listKeyPtr);

b32
basic_list_set_data_info(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr,
    p64 dataByteOffset, u64 dataByteSize);

b32
basic_list_get_data_info(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr,
    p64 *dataByteOffset, u64 *dataByteSize);

b32
basic_list_get_database(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *outDatabaseKeyPtr,
    p64 *outDataByteOffset, u64 *outDataByteSize);

b32
basic_list_reset_data(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *databaseKeyPtr, p64 dataByteOffset,
    u64 dataByteSize);

b32
basic_list_append(const struct memory_allocation_key *listKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
basic_list_insert_front(const struct memory_allocation_key *listKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
basic_list_append_after(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
basic_list_insert_before(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
basic_list_free_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr);

b32
basic_list_clear_nodes(const struct memory_allocation_key *listKeyPtr);

b32
basic_list_destroy(const struct memory_allocation_key *listKeyPtr);

#endif