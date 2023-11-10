
#pragma once

#include <types.h>

#include "allocator.h"

//
// struct user_hash_type { foo Key; boo Value; };
// user_hash_type* UserHashmap = nullptr;
//
// HashmapInit(UserHashmap);
// HashmapSetDefault(UserHashmap, SameDefaultValue);
// HashmapGetValue(UserHashmap, SomeKey);
//

// Perform a 128bit Murmur Hash
u128 Hash128(void* Data, int DataSize);
template<typename T> u128 Hash128(T* Data) { return Hash128((void*)Data, sizeof(T)); }

// Perform a 64bit Murmur Hash
u64 Hash64(void* Data, int DataSize);
template<typename T> u64 Hash64(T* Data) { return Hash64((void*)Data, sizeof(T)); }

#define HashmapInit(Hashmap, Allocator)
#define HashmapSetCapacity(Hashmap, Capacity)
#define HashmapFree(Hashmap)
#define HashmapClear(Hashmap)
#define HashmapReset(Hashmap)

#define HashmapInsert(Hashmap, KeyValuePair)
#define HashmapInsertKV(Hashmap, Key, Value)
#define HashmapRemove(Hashmap, Key)

#define HashmapGetValue(Hashmap, Key)
#define HashmapHasKey(Hashmap, Key)
#define HashmapGetKey(Hashmap, Key)
#define HashmapGetKeyHash(Hashmap, Key)
#define HashmapGetIndex(Hashmap, Key)

//
// Memory Layout
// [precomp hashes][default type][hashmap header][user facing data]
//

struct hashmap_header
{
    u64*      PrecomputedHashes; // Pre-hashed entries in the list. Contains the index of the actual element.
    u64       Size;              // Total Number of entries in the list.
    u64       Capacity;          // Total possible amount of entries in the list.
    allocator Allocator;
};
