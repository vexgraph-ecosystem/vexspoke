#include "struct/chunked_list.h"

#include <string.h>

#include "atomic/atomic.h"
#include "atomic/spin.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/stride.h"
#include "oop/type.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ChunkedList (struct/chunked_list.c)
 * LEVEL: L2 — Behavior (container behavior API)
 * ============================================================================
 * Never-moved chunked list: rows live in stable chunk blocks, so a row address
 * handed out once stays valid for the list's life and a reader on any thread
 * resolves an index to a row with no lock. Sibling of struct/List, which grows
 * one contiguous buffer and therefore moves every element on growth.
 *
 * Storage: rowsPerChunk is the largest power of two <= chunkBytes / stride, so
 * lookup is chunk = index >> rowShift, off = index & rowMask (no divide).
 * chunkBytes is a BYTE budget (default 128 = one Apple Silicon cache line), so
 * a row at or above the budget owns a cache line and its atomics never false
 * share. Chunks and directory generations are 16-byte aligned by the arena
 * contract (MemoryHeader is 16 bytes, payload lengths round to 16).
 *
 * The directory is copy-on-write: growth allocates a larger generation, copies
 * only the chunk pointers into it, and publishes it — old generations stay
 * mapped and valid, so a racing reader that loaded the old pointer keeps
 * reading live memory. Directory capacity lives inside each generation so a
 * reader can never pair a new capacity with an old array. Chunk slots are
 * AtomicPtr: a chunk is published only after its rows are zeroed, so a reader
 * sees null ("not there yet") or a ready block, never a torn one.
 *
 * STRUCT FIELDS (Mirroring struct/chunked_list.h):
 * ----------------------------------------------------------------------------
 *   ChunkedList {
 *     // --- ChunkedList core (embed-first: a ChunkedList* is a Collection*) ---
 *     Collection collection; // activeCount/stride/elementClass/capacity; data stays NULL
 *     // --- ChunkedList chunk part (rows never move once published) ---
 *     AtomicPtr directory;   // current directory generation (COW)
 *     uint32_t chunkCount;   // published chunks (owner-updated under the gate)
 *     uint32_t rowsPerChunk; // rows per chunk (power of two, >= 1)
 *     uint32_t rowShift;     // log2(rowsPerChunk)
 *     uint32_t rowMask;      // rowsPerChunk - 1
 *     uint32_t chunkBytes;   // byte budget per chunk (>= 16; default 128)
 *     // --- ChunkedList gate part (cold mutation only: chunks + directory) ---
 *     SpinLock lock;         // serializes growth; try-locked, 100ms bounded
 *   }
 *
 * PRIVATE HELPERS (kept file-local, pure data + growth math only):
 * ----------------------------------------------------------------------------
 *   ChunkDir {                // one directory generation (COW snapshot unit)
 *     struct ChunkDir *prev;  // previous generation, freed at teardown
 *     uint32_t capacity;      // chunk slots in THIS generation
 *     uint32_t pad;           // explicit padding so chunks[] is 8-byte aligned
 *     AtomicPtr chunks[];     // published chunk pointers (flexible array)
 *   }
 *   pow2Rows(stride, chunkBytes)      // largest power-of-two rows for a budget
 *   directoryOf(self)                 // lock-free current generation load
 *   rowAt(self, index)                // lock-free stable row resolve
 *   dirGrow(self)                     // COW directory doubling (cold)
 *   chunkAdd(self)                    // allocate + publish one chunk (cold)
 *   ensureChunks(self, needChunks)    // bounded-lock growth gate
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - ChunkedList(elementClass)                    : ChunkedList_1(elementClass)
 *   - ChunkedList(elementClass, chunkBytes)        : ChunkedList_2(elementClass, chunkBytes)
 *   - ChunkedList(elementClass, stride, chunkBytes): ChunkedList_3(elementClass, stride, chunkBytes)
 *
 * Core Functions:
 *   - ChunkedList_free(self)
 *   - ChunkedList_addSlot(self)
 *   - ChunkedList_reserve(self, rows)
 *   - ChunkedList_packInto(self, dest, destCap, outRows, outTruncated)
 *   - ChunkedList_slot(self, index)
 *   - ChunkedList_getChunk(self, chunkIndex)
 *
 * Setters:
 *   - ChunkedList_setChunkBytes(self, chunkBytes)
 *
 * Getters:
 *   - ChunkedList_size(self)
 *   - ChunkedList_length(self)
 *   - ChunkedList_capacity(self)
 *   - ChunkedList_isEmpty(self)
 *   - ChunkedList_elementClassId(self)
 *   - ChunkedList_stride(self)
 *   - ChunkedList_getChunkCount(self)
 *   - ChunkedList_getRowsPerChunk(self)
 *   - ChunkedList_getChunkBytes(self)
 * ============================================================================
 */

// struct/chunked_list.c — never-moved chunked list implementation.

// Growth gate bound (the Bounded Wait Law): 100ms is ~6 frames of slack and
// only expires when another thread is wedged, never on a healthy path.
#define VEX_CHUNKED_LOCK_NANOS 100000000ll

// PRIVATE HELPERS

typedef struct ChunkDir {
    struct ChunkDir *prev; // previous generation (freed last at teardown)
    uint32_t capacity;     // chunk slots in THIS generation
    uint32_t pad;          // explicit padding so chunks[] is 8-byte aligned
    AtomicPtr chunks[];    // published chunk pointers (flexible array)
} ChunkDir;

// Largest power of two rows that fits a byte budget (min 1).
static uint32_t pow2Rows(size_t stride, uint32_t chunkBytes) {
    uint32_t rows = (uint32_t)(chunkBytes / stride);
    if (rows == 0u)
        return 1u;
    uint32_t p = 1u;
    while ((p << 1) <= rows)
        p <<= 1u;
    return p;
}

static uint32_t shiftOf(uint32_t rows) {
    uint32_t shift = 0;
    while ((1u << shift) < rows)
        shift++;
    return shift;
}

static ChunkDir *directoryOf(const ChunkedList *self) {
    return (ChunkDir*) AtomicPtr_get(&(*self).directory);
}

// Lock-free stable row resolve: null when the row is not published yet.
static uint8_t *rowAt(const ChunkedList *self, uint32_t index) {
    ChunkDir *dir = directoryOf(self);
    if (!dir)
        return nullptr;
    uint32_t ci = index >> (*self).rowShift;
    if (ci >= (*dir).capacity)
        return nullptr;
    uint8_t *chunk = (uint8_t*) AtomicPtr_get(&(*dir).chunks[ci]);
    if (!chunk)
        return nullptr;
    size_t stride = (*self).collection.stride;
    return chunk + (size_t)(index & (*self).rowMask) * stride;
}

// Copy-on-write directory doubling: publish a bigger generation, keep the old
// one valid forever (the arraycopy of the cold path — chunk pointers only).
static bool dirGrow(ChunkedList *self) {
    ChunkDir *old = directoryOf(self);
    uint32_t oldCap = old ? (*old).capacity : 0u;
    uint32_t newCap = oldCap == 0u ? VEX_CHUNKED_DIR_INIT : oldCap * 2u;
    size_t bytes = sizeof(ChunkDir) + (size_t)newCap * sizeof(AtomicPtr);
    ChunkDir *next = (ChunkDir*) Memory_alloc(TYPE_CHUNKED_LIST, bytes);
    if (!next)
        return false;

    (*next).prev = old;
    (*next).capacity = newCap;
    (*next).pad = 0u;
    memset(&(*next).chunks[0], 0, (size_t)newCap * sizeof(AtomicPtr));

    uint32_t have = (*self).chunkCount;
    for (uint32_t i = 0; i < have; i++)
        AtomicPtr_exchange(&(*next).chunks[i], AtomicPtr_get(&(*old).chunks[i]));

    AtomicPtr_exchange(&(*self).directory, next);
    return true;
}

// Allocate one chunk, zero its rows, then publish it (acquire/release handoff).
static bool chunkAdd(ChunkedList *self) {
    ChunkDir *dir = directoryOf(self);
    if (!dir || (*self).chunkCount >= (*dir).capacity) {
        if (!dirGrow(self))
            return false;
        dir = directoryOf(self);
        if (!dir)
            return false;
    }

    size_t bytes = (size_t)(*self).rowsPerChunk * (size_t)(*self).collection.stride;
    uint64_t chunkType = Type_make(PROJ_VEXSPOKE, FORM_ARRAY, (*self).collection.elementClass);
    uint8_t *chunk = (uint8_t*) Memory_alloc(chunkType, bytes);
    if (!chunk)
        return false;

    memset(chunk, 0, bytes);
    AtomicPtr_exchange(&(*dir).chunks[(*self).chunkCount], chunk);
    (*self).chunkCount++;
    (*self).collection.capacity = (*self).chunkCount * (*self).rowsPerChunk;
    return true;
}

// Growth gate: bounded 100ms try-lock, drop-degrade false on contention.
static bool ensureChunks(ChunkedList *self, uint32_t needChunks) {
    if (needChunks <= (*self).chunkCount)
        return true;
    if (!SpinLock_tryLockTimeout(&(*self).lock, VEX_CHUNKED_LOCK_NANOS))
        return false;

    bool ok = true;
    while ((*self).chunkCount < needChunks && ok)
        ok = chunkAdd(self);

    SpinLock_unlock(&(*self).lock);
    return ok;
}

static ChunkedList *instant(uint32_t elementClass, size_t stride, uint32_t chunkBytes) {
    if (stride == 0)
        stride = Stride_get(elementClass);
    if (stride == 0)
        stride = sizeof(void*);
    if (chunkBytes < 16u)
        chunkBytes = 16u;

    ChunkedList *self = (ChunkedList*) Memory_alloc(TYPE_CHUNKED_LIST, sizeof(ChunkedList));
    if (!self)
        return nullptr;

    Collection *c = (Collection*) self;
    (*c).typeId = TYPE_CHUNKED_LIST;
    (*c).activeCount = 0;
    (*c).elementClass = elementClass;
    (*c).stride = (uint32_t)stride;
    (*c).capacity = 0;
    (*c).head = 0;
    (*c).data = nullptr;

    uint32_t rows = pow2Rows(stride, chunkBytes);
    (*self).directory.value = nullptr;
    (*self).chunkCount = 0;
    (*self).rowsPerChunk = rows;
    (*self).rowShift = shiftOf(rows);
    (*self).rowMask = rows - 1u;
    (*self).chunkBytes = chunkBytes;
    (*self).lock = SPIN_LOCK_INIT;
    return self;
}

// CONSTRUCTORS

ChunkedList *ChunkedList_1(uint32_t elementClass) {
    return instant(elementClass, Stride_get(elementClass), VEX_CHUNKED_BYTES_DEFAULT);
}

ChunkedList *ChunkedList_2(uint32_t elementClass, uint32_t chunkBytes) {
    return instant(elementClass, Stride_get(elementClass), chunkBytes);
}

ChunkedList *ChunkedList_3(uint32_t elementClass, uint32_t stride, uint32_t chunkBytes) {
    return instant(elementClass, stride, chunkBytes);
}

// CORE FUNCTIONS

void ChunkedList_free(ChunkedList *self) {
    if (!self)
        return;
    ChunkDir *dir = directoryOf(self);
    if (dir) {
        uint32_t chunks = (*self).chunkCount;
        for (uint32_t i = 0; i < chunks; i++)
            Memory_free(AtomicPtr_get(&(*dir).chunks[i]));
    }
    while (dir) {
        ChunkDir *prev = (*dir).prev;
        Memory_free(dir);
        dir = prev;
    }
    Memory_free(self);
}

uint8_t *ChunkedList_addSlot(ChunkedList *self) {
    if (!self)
        return nullptr;

    Collection *c = (Collection*) self;
    uint32_t index = (*c).activeCount;
    uint32_t needChunks = (index >> (*self).rowShift) + 1u;
    if (!ensureChunks(self, needChunks))
        return nullptr;

    uint8_t *row = rowAt(self, index);
    if (!row)
        return nullptr;

    memset(row, 0, (*c).stride);
    (*c).activeCount = index + 1u;
    return row;
}

bool ChunkedList_reserve(ChunkedList *self, uint32_t rows) {
    if (!self)
        return false;
    uint32_t needChunks = (uint32_t)(((uint64_t)rows + (*self).rowsPerChunk - 1u) >> (*self).rowShift);
    return ensureChunks(self, needChunks);
}

bool ChunkedList_packInto(const ChunkedList *self, uint8_t *dest, size_t destCap, uint32_t *outRows, bool *outTruncated) {
    if (outRows)
        *outRows = 0u;
    if (outTruncated)
        *outTruncated = false;
    if (!self || !dest)
        return false;

    uint32_t rows = (*self).collection.activeCount;
    size_t stride = (*self).collection.stride;
    size_t fit = stride == 0 ? 0 : destCap / stride;
    bool truncated = (size_t)rows > fit;
    uint32_t copy = truncated ? (uint32_t)fit : rows;

    uint8_t *out = dest;
    for (uint32_t i = 0; i < copy; i++) {
        uint8_t *row = rowAt(self, i);
        if (!row) {
            copy = i;
            truncated = true;
            break;
        }
        memcpy(out, row, stride);
        out += stride;
    }

    if (outRows)
        *outRows = copy;
    if (outTruncated)
        *outTruncated = truncated;
    return !truncated;
}

uint8_t *ChunkedList_slot(const ChunkedList *self, uint32_t index) {
    if (!self || index >= (*self).collection.activeCount)
        return nullptr;
    return rowAt(self, index);
}

uint8_t *ChunkedList_getChunk(const ChunkedList *self, uint32_t chunkIndex) {
    ChunkDir *dir = self ? directoryOf(self) : nullptr;
    if (!dir || chunkIndex >= (*dir).capacity)
        return nullptr;
    return (uint8_t*) AtomicPtr_get(&(*dir).chunks[chunkIndex]);
}

// SETTERS

void ChunkedList_setChunkBytes(ChunkedList *self, uint32_t chunkBytes) {
    if (!self || (*self).chunkCount > 0u)
        return; // budget is locked in once the first chunk exists (reject)
    if (chunkBytes < 16u)
        chunkBytes = 16u;
    uint32_t rows = pow2Rows((size_t)(*self).collection.stride, chunkBytes);
    (*self).chunkBytes = chunkBytes;
    (*self).rowsPerChunk = rows;
    (*self).rowShift = shiftOf(rows);
    (*self).rowMask = rows - 1u;
}

// GETTERS

uint32_t ChunkedList_size(const ChunkedList *self) {
    return self ? (*self).collection.activeCount : 0u;
}

uint32_t ChunkedList_length(const ChunkedList *self) {
    return ChunkedList_size(self);
}

uint32_t ChunkedList_capacity(const ChunkedList *self) {
    return self ? (*self).collection.capacity : 0u;
}

bool ChunkedList_isEmpty(const ChunkedList *self) {
    return self ? ((*self).collection.activeCount == 0u) : true;
}

uint32_t ChunkedList_elementClassId(const ChunkedList *self) {
    return self ? (*self).collection.elementClass : 0u;
}

uint32_t ChunkedList_stride(const ChunkedList *self) {
    return self ? (*self).collection.stride : 0u;
}

uint32_t ChunkedList_getChunkCount(const ChunkedList *self) {
    return self ? (*self).chunkCount : 0u;
}

uint32_t ChunkedList_getRowsPerChunk(const ChunkedList *self) {
    return self ? (*self).rowsPerChunk : 0u;
}

uint32_t ChunkedList_getChunkBytes(const ChunkedList *self) {
    return self ? (*self).chunkBytes : 0u;
}
