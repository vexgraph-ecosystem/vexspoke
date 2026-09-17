#ifndef STRUCT_CHUNKED_LIST_H
#define STRUCT_CHUNKED_LIST_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "atomic/spin.h"
#include "struct/collection.h"

// struct/chunked_list.h — the ChunkedList class: a never-moved chunked list.
//
// Sibling of struct/List (which grows one contiguous stride buffer and therefore
// MOVES every element on growth). A ChunkedList stores rows in never-moved
// chunks: a row's address is stable for the list's whole life, so a pointer
// handed out once (or an address patched into code) stays valid until free.
//
// CONCURRENCY CONTRACT (read before sharing across threads):
//   - addSlot and reserve are safe from any number of writer threads: the gate
//     serializes the whole claim-plus-growth-plus-commit step, so every claimed
//     row is handed out exactly once.
//   - slot, getChunk, size, length, capacity, isEmpty, getChunkCount and
//     packInto are lock-free and safe concurrently with writers. A reader that
//     observes size N is guaranteed slot(i) resolves for every i below N (a
//     null there is a publication defect, never a race). Sizes are monotonic
//     observations: a later read never reports fewer rows.
//   - setChunkBytes is setup-time only: call it before the list is shared and
//     never concurrently with any other call. Geometry is plain state.
//   - free requires quiescence: no concurrent readers or writers.
//   - The embedded Collection mirror (activeCount/capacity) is maintained under
//     the gate for single-threaded Collection_* use. While a writer is active,
//     read counts only through the ChunkedList_* getters, which load atomics.
//
// Storage shape (the Data-Oriented Storage Law + the Dynamic Scalability &
// Anti-Hardcoding Law):
//   - rowsPerChunk = largest power of two <= chunkBytes / stride (min 1), so
//     lookup is chunk = index >> rowShift, off = index & rowMask (no divide).
//     All size math is checked: growth that cannot be represented fails closed
//     instead of wrapping.
//   - chunkBytes is a BYTE budget, not a row count: default 128 (sized to one
//     Apple Silicon cache line). A row at or above the budget gets a chunk to
//     itself, so neighbouring rows never share that chunk's bytes. This is NOT
//     a cache-line isolation guarantee: the arena aligns payloads to 16 bytes,
//     not 128, so two chunks may still land on one hardware line.
//   - the directory is copy-on-write and its generations are NEVER freed while
//     the list runs: a grow publishes a bigger generation and leaves the old
//     one valid, so a racing lock-free reader that loaded the old pointer keeps
//     reading live memory (the classic COW snapshot rule). Directory capacity
//     lives inside each generation for the same reason — a reader must never
//     pair a new capacity with an old array.
//   - chunk slots are published via atomic pointers: publishing a chunk is an
//     acquire/release handoff, so a reader sees either null ("not there yet")
//     or a fully initialized row block — never a torn one. The committed count
//     is stored only after the chunk is published, so index < committed implies
//     the row is resolvable.
// Growth copies nothing but the directory's chunk pointers (the cold
// arraycopy), and that copy happens only when the directory doubles.
//
// Because the embedded Collection is the first member, a ChunkedList pointer is
// also a Collection pointer — Collection_* accessors work on it directly while
// single-threaded or quiesced (see the contract above).
// Collection.data stays NULL by design: there is no single contiguous buffer.
//
// Use it where addresses must be stable or readers are lock-free (hot-reload
// trampoline registries, module registries, bridge tables). Use struct/List
// where the container is private to one thread and index access is all that
// matters.

// Byte budget per chunk. 128 sizes one chunk to one Apple Silicon cache line;
// the arena's alignment guarantee stays 16 bytes (see the contract above).
#define VEX_CHUNKED_BYTES_DEFAULT 128u

// Chunk slots in the first directory generation (512 bytes, one COW step).
#define VEX_CHUNKED_DIR_INIT 64u

typedef struct ChunkedList {
    // --- ChunkedList core (embed-first: a ChunkedList* is a Collection*) ---
    Collection collection; // gate-maintained mirror; quiesced reads only under concurrency
    // --- ChunkedList chunk part (rows never move once published) ---
    _Atomic(void*) directory;  // current directory generation (COW, generations never freed mid-run)
    _Atomic uint32_t chunkCount; // published chunks (lock-free reader bound)
    _Atomic uint32_t committed;  // published rows (lock-free reader bound)
    uint32_t rowsPerChunk;     // rows per chunk (power of two, >= 1; setup-time geometry)
    uint32_t rowShift;         // log2(rowsPerChunk)
    uint32_t rowMask;          // rowsPerChunk - 1
    uint32_t chunkBytes;       // byte budget per chunk (>= 16; default 128)
    // --- ChunkedList gate part (serializes every mutation step) ---
    SpinLock lock;             // claim + growth + commit; try-locked, 100ms bounded
} ChunkedList;

// Constructors
ChunkedList *ChunkedList_1(uint32_t elementClass);
ChunkedList *ChunkedList_2(uint32_t elementClass, uint32_t chunkBytes);
// Explicit stride for row layouts the Struct registry does not know
// (e.g. a graphvex-side row struct holding its own atomics).
ChunkedList *ChunkedList_3(uint32_t elementClass, uint32_t stride, uint32_t chunkBytes);

// Core functions
// Requires quiescence: no concurrent readers or writers.
void ChunkedList_free(ChunkedList *self);
// Append a zeroed row and return its stable address. Thread-safe: the gate
// serializes claim, growth and commit, so every row is handed out exactly
// once. Null on OOM, lock timeout, or index exhaustion. The returned pointer
// stays valid until ChunkedList_free.
uint8_t *ChunkedList_addSlot(ChunkedList *self);
// Pre-allocate room for `rows` without activating any. Thread-safe growth gate
// (cold path). False on OOM or lock timeout; the list is left untouched.
bool ChunkedList_reserve(ChunkedList *self, uint32_t rows);
// Snapshot rows into a flat caller buffer (the arraycopy seam, dest-last per
// the Dest-Last Law): false + *outTruncated when destCap cannot hold every
// active row (the Truncation-Never-Silent clause). Lock-free; safe
// concurrently with writers (copies a prefix of the published rows).
bool ChunkedList_packInto(const ChunkedList *self, uint8_t *dest, size_t destCap, uint32_t *outRows, bool *outTruncated);
// Stable row address, lock-free: null when the index is at or above the
// published count. A null below the observed size is a publication defect.
uint8_t *ChunkedList_slot(const ChunkedList *self, uint32_t index);
// Chunk base address, lock-free: null when the chunk is not published.
uint8_t *ChunkedList_getChunk(const ChunkedList *self, uint32_t chunkIndex);

// Setters
// Byte budget for future chunks. Setup-time only: never concurrently with any
// other call. No-op once the first chunk exists or the gate is contended.
void ChunkedList_setChunkBytes(ChunkedList *self, uint32_t chunkBytes);

// Getters (all lock-free atomic loads; safe concurrently with writers)
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
