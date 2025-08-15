#ifndef __UNIQUE_LIST_H
#define __UNIQUE_LIST_H

#include "types.h"
#include "basic_list.h"
#include "memory.h"

b32
unique_list_create(const struct memory_page_key *pageKeyPtr, 
    p64 dataByteOffset, u64 dataByteSize, 
    const struct memory_allocation_key *databaseKeyPtr, 
    const struct memory_allocation_key *outListKeyPtr);

b32
unique_list_get_node_id(const struct memory_allocation_key *nodeKeyPtr, 
    basic_list_id *outIdPtr);

b32
unique_list_get_node_by_id(const struct memory_allocation_key *listKeyPtr, 
    basic_list_id nodeId, 
    const struct memory_allocation_key *outNodekeyPtr);

b32
unique_list_get_is_node_active(const struct memory_allocation_key *nodeKeyPtr);

b32
unique_list_move_node(const struct memory_allocation_key *lhsListKeyPtr, 
    const struct memory_allocation_key *rhsListKeyPtr,
    const struct memory_allocation_key *nodeKeyPtr);

b32
unique_list_move_free_node(const struct memory_allocation_key *lhsListKeyPtr,
    const struct memory_allocation_key *rhsListKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
unique_list_get_active_count(const struct memory_allocation_key *listKeyPtr, 
    u64 *outActiveCount);

b32
unique_list_get_next_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outNodeKeyPtr);

b32
unique_list_get_prev_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outNodeKeyPtr);

b32
unique_list_map_data(const struct memory_allocation_key *nodeKeyPtr, void **outDataPtr);

b32
unique_list_unmap_data(const struct memory_allocation_key *listKeyPtr);

b32
unique_list_set_data_info(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr,
    p64 dataByteOffset, u64 dataByteSize);

b32
unique_list_get_data_info(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr,
    p64 *dataByteOffset, u64 *dataByteSize);

b32
unique_list_get_database(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *outDatabaseKeyPtr,
    p64 *outDataByteOffset, u64 *outDataByteSize);

b32
unique_list_reset_data(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *databaseKeyPtr, p64 dataByteOffset,
    u64 dataByteSize);

b32
unique_list_append(const struct memory_allocation_key *listKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
unique_list_insert_front(const struct memory_allocation_key *listKeyPtr,
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
unique_list_append_after(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
unique_list_insert_before(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *lhsNodeKeyPtr, 
    p64 dataByteOffset,
    u64 dataByteSize,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
unique_list_free_node(const struct memory_allocation_key *listKeyPtr, 
    const struct memory_allocation_key *nodeKeyPtr);

b32
unique_list_clear_nodes(const struct memory_allocation_key *listKeyPtr);

b32
unique_list_destroy(const struct memory_allocation_key *listKeyPtr);

#endif