#ifndef STRUCT_COLLECTION_H
#define STRUCT_COLLECTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "annotation/debug.h"
#include "annotation/test.h"

// struct/collection.h — the Collection metadata struct, ported from
// struct/Collection.java.
//
// Every collection (List, Array, Stack, Deque, Queue, Map, Set) embeds a
// Collection as its first member, so a pointer to any of them is also a
// Collection pointer and the accessors below read the shared header. The type
// itself lives in the block header (Memory_type), so Collection_type() walks
// back to it — one subtract, nothing more.

typedef struct Collection {
    uint64_t typeId;        // mirror of the block-header type (for debug)
    uint32_t activeCount;   // number of live elements
    uint32_t elementClass;  // class of elements (Map: key class)
    uint32_t stride;        // Bytes per element (Map: val class)
    uint32_t capacity;      // element capacity (or slot capacity)
    uint32_t head;          // circular head index (Deque/Queue); else 0
    uint8_t *data;          // element / slot buffer
    // Exhaustion observability (the Exhaustion Loudness Law): every refused
    // grow is counted; the epoch's first refusal also emits one diagnostic.
    uint64_t exhaustionCount;   // refusals since construction/reset
    bool exhaustionReported;    // the epoch's one diagnostic was emitted
} Collection;

uint64_t Collection_type(Collection *c);
uint32_t Collection_size(Collection *c);
uint32_t Collection_length(Collection *c);
bool Collection_isEmpty(Collection *c);
uint32_t Collection_elementClassId(Collection *c);
uint32_t Collection_keyClassId(Collection *c);
uint32_t Collection_stride(Collection *c);
uint32_t Collection_valClassId(Collection *c);
uint32_t Collection_capacity(Collection *c);
uint32_t Collection_head(Collection *c);
uint8_t *Collection_dataBuffer(Collection *c);

// --- Exhaustion observability (the Exhaustion Loudness Law) ---
// Called by a container when its buffer could not grow: counts the refusal and
// emits the epoch's single THROW naming the requested bytes. Always returns
// false so a caller can `return Collection_reportExhaustion(...)`. A collection
// constructed (or reset) starts a fresh epoch.
;;DEBUG
bool Collection_reportExhaustion(Collection *c, size_t requestedBytes);
// The number of refused grows since the collection was constructed or reset.
;;TEST
uint64_t Collection_exhaustionCount(const Collection *c);
// Start a fresh exhaustion epoch (count zeroed, diagnostic armed again).
;;TEST
void Collection_resetExhaustion(Collection *c);

// Generic slot read/write for the scalar/pointer collections. Reads the slot at
// index with the collection's stride, widening small ints (legacy readSlot).
uint64_t Collection_readSlot(Collection *c, size_t index);
void Collection_writeSlot(Collection *c, size_t index, uint64_t value);

// Slot read/write without bounds checks (legacy unsafeGet/setUnsafe).
uint64_t Collection_readSlotUnsafe(Collection *c, size_t index);
void Collection_writeSlotUnsafe(Collection *c, size_t index, uint64_t value);

#endif