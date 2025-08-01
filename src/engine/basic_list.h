#ifndef __BASIC_LIST_H
#define __BASIC_LIST_H

#include "types.h"
#include "memory.h"

b32
basic_list_create(const struct memory_page_key *memoryPageKeyPtr, 
    const struct memory_allocation_key *outListKeyPtr);

b32
basic_list_get_data(const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *outDataKeyPtr);

b32
basic_list_set_data(const struct memory_allocation_key *nodeKeyPtr, 
    const struct memory_allocation_key *dataKeyPtr);

b32
basic_list_append(const struct memory_allocation_key *listKeyPtr,
    const struct memory_allocation_key *dataKeyPtr,
    const struct memory_allocation_key *outNodeKeyPtr);

b32
basic_list_insert_front(const struct memory_allocation_key *memoryAllocationKeyPtr,
    const struct basic_list_node *outNodePtr);

b32
basic_list_append_after(const struct memory_allocation_key *memoryAllocationKeyPtr,
    const struct basic_list_node *lhsNodePtr, const struct basic_list_node *outRhsNodePtr);

b32
basic_list_insert_before(const struct memory_allocation_key *memoryAllocationKeyPtr,
    const struct basic_list_node *lhsNodePtr, const struct basic_list_node *outRhsNodePtr);

b32
basic_list_get_allocation(const struct basic_list_node *nodePtr, 
    const struct memory_allocation_key *memoryAllocationKeyPtr);

b32
basic_list_remove_node(const struct basic_list_node *nodePtr);

#endif