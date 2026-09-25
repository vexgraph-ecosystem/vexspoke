#ifndef STRUCT_CHUNKED_LIST_H
#define STRUCT_CHUNKED_LIST_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "atomic/spin.h"
#include "struct/collection.h"

// struct/chunked_list.h — the ChunkedList class: a never-moved radix-paged list.
//
// Sibling of struct/List (which grows one contiguous stride buffer and therefore
// MOVES every element on growth). A ChunkedList stores rows in never-moved leaf
// blocks reached through a RADIX PAGE TABLE: a growable root, any number of
// fixed-radix internal levels, and a page-sized leaf. A row's address is stable
// for the list's whole life, so a pointer handed out once (or an address patched
// into code) stays valid until free.
//
// The page walk is a bounded number of dependent loads, not a linked list: the
// root is a flat array, every internal node is a flat array, and a leaf is a
// contiguous run of rows. Radices are powers of two, so every hop is shift/mask
// (chunk = index >> shift, digit = (index >> shift) & mask) — no divide, no
// branch. Leaf rows default to one page (4096 / stride), so one leaf is one
// streaming unit and one address-translation entry. This is a data-oriented
// page table, not pointer chasing (the Data-Oriented Storage Law).
//
// TWO WAYS TO BUILD IT:
//   1. Byte budget (the historical form): ChunkedList_1/_2/_3(elementClass,
//      [stride,] chunkBytes). Always a 2-level table (root -> leaf).
//   2. Radix form (the variadic macro): ChunkedList(elementClass, stride,
//      r0, r1, ..., leafRows) — any number of levels; the LAST argument is the
//      leaf row count and the rest are internal fan-outs. The root is growable,
//      so there is no capacity ceiling (the Dynamic Scalability &
//      Anti-Hardcoding Law). Pass a *_LAYER_DEFAULT sentinel for a preset
//      shape (see below).
//
// CONCURRENCY CONTRACT (read before sharing across threads):
//   - addSlot and reserve are safe from any number of writer threads: the gate
//     serializes the whole claim-plus-growth-plus-commit step, so every claimed
//     row is handed out exactly once.
//   - slot, getChunk, size, length, capacity, isEmpty, getChunkCount and
//     packInto are lock-free and safe concurrently with writers. A reader that
//     observes size N is guaranteed slot(i) resolves for every i below N (a
//     null there is a publication defect, never a race).
//   - setChunkBytes is setup-time only: call it before the list is shared and
//     never concurrently with any other call.
//   - free requires quiescence: no concurrent readers or writers.
//   - The embedded Collection mirror (activeCount/capacity) is maintained under
//     the gate for single-threaded Collection_* use. While a writer is active,
//     read counts only through the ChunkedList_* getters, which load atomics.
//
// Storage shape:
//   - the ROOT is a growable copy-on-write generation whose generations are
//     NEVER freed while the list runs: a grow publishes a bigger generation and
//     leaves the old one valid, so a racing lock-free reader that loaded the
//     old pointer keeps reading live memory (the classic COW snapshot rule).
//   - INTERNAL nodes are allocated once and never moved: they are published into
//     their parent slot with an acquire/release handoff, so a reader sees either
//     null ("not there yet") or a fully initialized node, never a torn one.
//   - LEAVES hold rows and are published the same way; the committed count is
//     stored only after the row's leaf is published, so index < committed
//     implies the row is resolvable.
//   - chunkBytes is a BYTE budget for the byte-budget form, a page-sized leaf
//     by default (128 bytes sized one chunk to one Apple Silicon cache line; a
//     radix-form leaf sizes to one page). A row at or above the leaf budget gets
//     a leaf to itself.
//
// Because the embedded Collection is the first member, a ChunkedList pointer is
// also a Collection pointer — Collection_* accessors work on it directly while
// single-threaded or quiesced (see the contract above). Collection.data stays
// NULL by design: there is no single contiguous buffer.
//
// Use it where addresses must be stable or readers are lock-free (hot-reload
// trampoline registries, module registries, bridge tables, the reflection
// shelf). Use struct/List where the container is private to one thread and
// index access is all that matters.

// Byte budget per chunk for the byte-budget form. 128 sizes one chunk to one
// Apple Silicon cache line; the arena's alignment guarantee stays 16 bytes.
#define VEX_CHUNKED_BYTES_DEFAULT 128u

// Initial root generation slots (512 bytes, one COW step).
#define VEX_CHUNKED_DIR_INIT 64u

// Default leaf target for the radix form: one page of rows. The leaf radix is
// the largest power of two <= VEX_CHUNKED_PAGE_BYTES / stride (min 1).
#define VEX_CHUNKED_PAGE_BYTES 4096u

// Default internal fan-out for the radix form (512 pointers = one page; used
// when a *_LAYER_DEFAULT sentinel fills the internal levels).
#define VEX_CHUNKED_INTERNAL_RADIX_DEFAULT 512u

// Radix-form shape sentinels. Passed where a fan-out would go, they select a
// preset shape (see ChunkedList_paged). Negative on purpose.
//
// INTENTIONAL(vex): the *_LAYER_DEFAULT sentinels are FourCCs with the high bit
// of the first byte set ('T' | 0x80 = 0xD4 = Ô), so they read NEGATIVE as
// int32_t on every platform and can never collide with a real fan-out — the
// same author sugar as SIZE_AUTO ("åuto", 0xE575746F). Do not "normalize" them
// to round numbers; the sign IS the sentinel.
#define CHUNKED_LIST_TWO_LAYER_DEFAULT ((int32_t) 0xD4574F21)   // "ÔWO!"
#define CHUNKED_LIST_THREE_LAYER_DEFAULT ((int32_t) 0xD4485245) // "ÔHRE"

typedef struct ChunkedList {
    // --- ChunkedList core (embed-first: a ChunkedList* is a Collection*) ---
    Collection collection;     // gate-maintained mirror; quiesced reads only under concurrency
    // --- ChunkedList paging part (radix page table; rows never move) ---
    _Atomic(void*) directory;  // current root generation (COW; generations never freed mid-run)
    _Atomic uint32_t chunkCount; // published leaves (lock-free reader bound)
    _Atomic uint32_t committed;  // published rows (lock-free reader bound)
    uint32_t rowsPerChunk;     // rows per leaf (power of two, >= 1; geometry)
    uint32_t rowShift;         // log2(rowsPerChunk)
    uint32_t rowMask;          // rowsPerChunk - 1
    uint32_t chunkBytes;       // byte budget of one leaf (>= 16)
    uint32_t levels;           // pointer hops root..leaf (>= 2)
    uint32_t *radices;         // [levels] fan-out per depth; [0] = initial root slots
    uint32_t *shifts;          // [levels] bit offset of each depth's digit ([levels-1] == 0)
    uint32_t *masks;           // [levels] radix-1 per depth ([0] unused: the root is growable)
    // --- ChunkedList gate part (serializes every mutation step) ---
    SpinLock lock;             // claim + growth + commit; try-locked, 100ms bounded
} ChunkedList;

// Constructors (byte-budget form: a 2-level table)
ChunkedList *ChunkedList_1(uint32_t elementClass);
ChunkedList *ChunkedList_2(uint32_t elementClass, uint32_t chunkBytes);
// Explicit stride for row layouts the Struct registry does not know
// (e.g. a graphvex-side row struct holding its own atomics).
ChunkedList *ChunkedList_3(uint32_t elementClass, uint32_t stride, uint32_t chunkBytes);

// Radix form: build a table of `count` levels from `radices[]` (outer to leaf).
// radices[0] is the initial root slot count (the root still grows); the LAST
// radix is the leaf row count. A negative sentinel anywhere selects a preset:
//   CHUNKED_LIST_TWO_LAYER_DEFAULT   -> { rootInit, pageLeaf }
//   CHUNKED_LIST_THREE_LAYER_DEFAULT -> { rootInit, internal, pageLeaf }
// Non-power-of-two radices are rounded DOWN to a power of two (min 1). `stride`
// 0 falls back to Stride_get(elementClass), then sizeof(void*). Page leaf =
// largest power of two <= VEX_CHUNKED_PAGE_BYTES / stride.
ChunkedList *ChunkedList_paged(uint32_t elementClass, uint32_t stride, const int32_t *radices, size_t count);

// Variadic convenience: ChunkedList(elementClass, stride, r0, ..., leafRows).
// At least one radix/sentinel is required.
#define ChunkedList(elementClass, stride, ...)                                   \
    ChunkedList_paged((elementClass), (stride),                                  \
        (const int32_t[]){ __VA_ARGS__ },                                        \
        sizeof((const int32_t[]){ __VA_ARGS__ }) / sizeof(int32_t))

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
// Leaf base address, lock-free: null when the leaf is not published.
uint8_t *ChunkedList_getChunk(const ChunkedList *self, uint32_t chunkIndex);

// Setters
// Byte budget for the leaf. Setup-time only: never concurrently with any other
// call. No-op once the first leaf exists or the gate is contended.
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
// Number of pointer hops from the root to a leaf (2 for the byte-budget form).
uint32_t ChunkedList_getLevels(const ChunkedList *self);

#endif
