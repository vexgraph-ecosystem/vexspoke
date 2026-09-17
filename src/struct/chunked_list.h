#ifndef STRUCT_CHUNKED_LIST_H
#define STRUCT_CHUNKED_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "atomic/atomic.h"
#include "atomic/spin.h"
#include "struct/collection.h"

// struct/chunked_list.h — the ChunkedList class: a never-moved chunked list.
//
// Sibling of struct/List (which grows one contiguous stride buffer and therefore
// MOVES every element on growth). A ChunkedList stores rows in never-moved
// chunks: a row's address is stable for the list's whole life, so a pointer
// handed out once (or an address patched into code) stays valid forever, and a
// reader on any thread can resolve an index to a row without a lock.
//
// Storage shape (the Data-Oriented Storage Law + the Dynamic Scalability &
// Anti-Hardcoding Law):
//   - rowsPerChunk = largest power of two <= chunkBytes / stride (min 1), so
//     lookup is chunk = index >> rowShift, off = index & rowMask (no divide).
//   - chunkBytes is a BYTE budget, not a row count: default 128 (one Apple
//     Silicon cache line). A row >= the budget gets its own cache line, so
//     per-row atomics never false-share with a neighbour row.
//   - chunks and every directory generation are 16-byte aligned by the arena
//     contract (MemoryHeader is 16 bytes, payload lengths round to 16).
//   - the directory is copy-on-write and its generations are NEVER freed while
//     the list runs: a grow publishes a bigger generation and leaves the old
//     one valid, so a racing lock-free reader that loaded the old pointer keeps
//     reading live memory (the classic COW snapshot rule). Directory capacity
//     lives inside each generation for the same reason — a reader must never
//     pair a new capacity with an old array.
//   - chunk slots are AtomicPtr: publishing a chunk is an acquire/release
//     handoff, so a reader sees either null ("not there yet") or a fully
//     initialized row block — never a torn one.
// Growth copies nothing but the directory's chunk pointers (the cold
// arraycopy), and that copy happens only when the directory doubles.
//
// Because the embedded Collection is the first member, a ChunkedList pointer is
// also a Collection pointer — Collection_* accessors work on it directly.
// Collection.data stays NULL by design: there is no single contiguous buffer.
//
// Use it where addresses must be stable or readers are lock-free (hot-reload
// trampoline registries, module registries, bridge tables). Use struct/List
// where the container is private to one thread and index access is all that
// matters.

// Byte budget per chunk. 128 = one Apple Silicon cache line; the arena's
// 16-byte alignment contract is met by construction.
#define VEX_CHUNKED_BYTES_DEFAULT 128u

// Chunk slots in the first directory generation (512 bytes, one COW step).
#define VEX_CHUNKED_DIR_INIT 64u

typedef struct ChunkedList {
    // --- ChunkedList core (embed-first: a ChunkedList* is a Collection*) ---
    Collection collection; // activeCount/stride/elementClass/capacity; data stays NULL
    // --- ChunkedList chunk part (rows never move once published) ---
    AtomicPtr directory;   // current directory generation (COW, generations never freed mid-run)
    uint32_t chunkCount;   // published chunks (owner-updated under the gate)
    uint32_t rowsPerChunk; // rows per chunk (power of two, >= 1)
    uint32_t rowShift;     // log2(rowsPerChunk)
    uint32_t rowMask;      // rowsPerChunk - 1
    uint32_t chunkBytes;   // byte budget per chunk (>= 16; default 128)
    // --- ChunkedList gate part (cold mutation only: chunks + directory) ---
    SpinLock lock;         // serializes growth; try-locked, 100ms bounded
} ChunkedList;

// Constructors
ChunkedList *ChunkedList_1(uint32_t elementClass);
ChunkedList *ChunkedList_2(uint32_t elementClass, uint32_t chunkBytes);
// Explicit stride for row layouts the Struct registry does not know
// (e.g. a graphvex-side row struct holding its own atomics).
ChunkedList *ChunkedList_3(uint32_t elementClass, uint32_t stride, uint32_t chunkBytes);

// Core functions
void ChunkedList_free(ChunkedList *self);
// Append a zeroed row and return its stable address (null on OOM or a lock
// timeout). The returned pointer stays valid until ChunkedList_free.
uint8_t *ChunkedList_addSlot(ChunkedList *self);
// Pre-allocate room for `rows` without activating any (cold path).
bool ChunkedList_reserve(ChunkedList *self, uint32_t rows);
// Snapshot rows into a flat caller buffer (the arraycopy seam, dest-last per
// the Dest-Last Law): false + *outTruncated when destCap cannot hold every
// active row (the Truncation-Never-Silent clause).
bool ChunkedList_packInto(const ChunkedList *self, uint8_t *dest, size_t destCap, uint32_t *outRows, bool *outTruncated);
// Stable row address, lock-free: null when the row is not allocated.
uint8_t *ChunkedList_slot(const ChunkedList *self, uint32_t index);
// Chunk base address, lock-free: null when the chunk is not published.
uint8_t *ChunkedList_getChunk(const ChunkedList *self, uint32_t chunkIndex);

// Setters
// Byte budget for future chunks; no-op once the first chunk exists.
void ChunkedList_setChunkBytes(ChunkedList *self, uint32_t chunkBytes);

// Getters
uint32_t ChunkedList_size(const ChunkedList *self);
uint32_t ChunkedList_length(const ChunkedList *self);
uint32_t ChunkedList_capacity(const ChunkedList *self);
bool ChunkedList_isEmpty(const ChunkedList *self);
uint32_t ChunkedList_elementClassId(const ChunkedList *self);
uint32_t ChunkedList_stride(const ChunkedList *self);
uint32_t ChunkedList_getChunkCount(const ChunkedList *self);
uint32_t ChunkedList_getRowsPerChunk(const ChunkedList *self);
uint32_t ChunkedList_getChunkBytes(const ChunkedList *self);

#define ChunkedList(...) CONSTRUCTOR_DISPATCH(ChunkedList, __VA_ARGS__)

#endif
