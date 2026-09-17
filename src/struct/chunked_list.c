#include "struct/chunked_list.h"

#include <stdatomic.h>
#include <string.h>

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
 * handed out once stays valid until free. Sibling of struct/List, which grows
 * one contiguous buffer and therefore moves every element on growth.
 *
 * Concurrency: addSlot and reserve are safe from any number of writers — the
 * gate serializes the whole claim-plus-growth-plus-commit step, so every row
 * is handed out exactly once. slot, getChunk, the count getters and packInto
 * are lock-free and safe concurrently with writers: the committed count is
 * stored only after the row's chunk is published, so index < committed implies
 * the row resolves. setChunkBytes is setup-time only and free requires
 * quiescence. The embedded Collection mirror is maintained under the gate for
 * single-threaded Collection_* use; under concurrency read counts only through
 * the ChunkedList_* getters.
 *
 * Storage: rowsPerChunk is the largest power of two <= chunkBytes / stride, so
 * lookup is chunk = index >> rowShift, off = index & rowMask (no divide). All
 * size math is checked and fails closed instead of wrapping. chunkBytes is a
 * BYTE budget (default 128, sized to one Apple Silicon cache line): a row at
 * or above the budget gets a chunk to itself. That is not a cache-line
 * isolation guarantee — the arena aligns payloads to 16 bytes, not 128.
 *
 * The directory is copy-on-write: growth allocates a larger generation, copies
 * only the chunk pointers into it, and publishes it — old generations stay
 * mapped and valid, so a racing reader that loaded the old pointer keeps
 * reading live memory. Directory capacity lives inside each generation so a
 * reader can never pair a new capacity with an old array. Chunk slots are
 * atomic pointers: a chunk is published only after its rows are zeroed, so a
 * reader sees null ("not there yet") or a ready block, never a torn one.
 *
 * STRUCT FIELDS (Mirroring struct/chunked_list.h):
 * ----------------------------------------------------------------------------
 *   ChunkedList {
 *     // --- ChunkedList core (embed-first: a ChunkedList* is a Collection*) ---
 *     Collection collection; // gate-maintained mirror; quiesced reads only under concurrency
 *     // --- ChunkedList chunk part (rows never move once published) ---
 *     _Atomic(void*) directory;  // current directory generation (COW)
 *     _Atomic uint32_t chunkCount; // published chunks (lock-free reader bound)
 *     _Atomic uint32_t committed;  // published rows (lock-free reader bound)
 *     uint32_t rowsPerChunk;     // rows per chunk (power of two, >= 1; setup-time geometry)
 *     uint32_t rowShift;         // log2(rowsPerChunk)
 *     uint32_t rowMask;          // rowsPerChunk - 1
 *     uint32_t chunkBytes;       // byte budget per chunk (>= 16; default 128)
 *     // --- ChunkedList gate part (serializes every mutation step) ---
 *     SpinLock lock;             // claim + growth + commit; try-locked, 100ms bounded
 *   }
 *
 * PRIVATE HELPERS (kept file-local, pure data + growth math only):
 * ----------------------------------------------------------------------------
 *   ChunkDir {                // one directory generation (COW snapshot unit)
 *     struct ChunkDir *prev;  // previous generation, freed at teardown
 *     uint32_t capacity;      // chunk slots in THIS generation
 *     uint32_t pad;           // explicit padding so chunks[] is 8-byte aligned
 *     _Atomic(uint8_t*) chunks[]; // published chunk pointers (flexible array)
 *   }
 *   pow2Rows(stride, chunkBytes)      // largest power-of-two rows for a budget (wrap-safe)
 *   shiftOf(rows)                     // log2 of a power-of-two row count
 *   directoryOf(self)                 // lock-free current generation load
 *   rowAt(self, index)                // lock-free stable row resolve
 *   dirGrow(self)                     // COW directory doubling (gate held, cold)
 *   chunkAddLocked(self)              // allocate + publish one chunk (gate held, cold)
 *   ensureChunksLocked(self, need)    // growth gate body (gate held, cold)
 *   ensureChunks(self, need)          // bounded-lock growth gate
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
    _Atomic(uint8_t*) chunks[]; // published chunk pointers (flexible array)
} ChunkDir;

// Largest power of two rows that fits a byte budget (min 1). The loop test
// divides instead of shifting so it cannot wrap on large budgets.
static uint32_t pow2Rows(size_t stride, uint32_t chunkBytes) {
    uint32_t rows = (uint32_t)(chunkBytes / stride);
    if (rows == 0u)
        return 1u;
    uint32_t p = 1u;
    while (p <= (rows >> 1))
        p <<= 1u;
    return p;
}

static uint32_t shiftOf(uint32_t rows) {
    if (rows == 0u)
        return 0u;
    uint32_t shift = 0;
    while ((1u << shift) < rows)
        shift++;
    return shift;
}

static ChunkDir *directoryOf(const ChunkedList *self) {
    return (ChunkDir*) atomic_load_explicit(&(*self).directory, memory_order_acquire);
}

// Lock-free stable row resolve: null when the row is not published yet.
// Callers bound index by the committed count first; the committed store lands
// only after the chunk publish, so index < committed implies non-null here.
static uint8_t *rowAt(const ChunkedList *self, uint32_t index) {
    ChunkDir *dir = directoryOf(self);
    if (!dir)
        return nullptr;
    uint32_t ci = index >> (*self).rowShift;
    if (ci >= (*dir).capacity)
        return nullptr;
    uint8_t *chunk = atomic_load_explicit(&(*dir).chunks[ci], memory_order_acquire);
    if (!chunk)
        return nullptr;
    size_t stride = (*self).collection.stride;
    return chunk + (size_t)(index & (*self).rowMask) * stride;
}

// Copy-on-write directory doubling: publish a bigger generation, keep the old
// one valid forever (the arraycopy of the cold path — chunk pointers only).
// Gate held by the caller: exactly one grower, so chunkCount only moves here.
static bool dirGrow(ChunkedList *self) {
    ChunkDir *old = directoryOf(self);
    uint32_t oldCap = old ? (*old).capacity : 0u;
    if (oldCap > (UINT32_MAX / 2u))
        return false;
    uint32_t newCap = oldCap == 0u ? VEX_CHUNKED_DIR_INIT : oldCap * 2u;
    size_t bytes = sizeof(ChunkDir) + (size_t)newCap * sizeof(_Atomic(uint8_t*));
    ChunkDir *next = (ChunkDir*) Memory_alloc(TYPE_CHUNKED_LIST, bytes);
    if (!next)
        return false;

    (*next).prev = old;
    (*next).capacity = newCap;
    (*next).pad = 0u;
    memset(&(*next).chunks[0], 0, (size_t)newCap * sizeof(_Atomic(uint8_t*)));

    uint32_t have = atomic_load(&(*self).chunkCount);
    if (have > newCap) {
        Memory_free(next);
        return false;
    }
    for (uint32_t i = 0; i < have; i++)
        atomic_store_explicit(&(*next).chunks[i], atomic_load_explicit(&(*old).chunks[i], memory_order_relaxed), memory_order_relaxed);

    atomic_store_explicit(&(*self).directory, next, memory_order_release);
    return true;
}

// Allocate one chunk, zero its rows, then publish it (acquire/release handoff).
// Gate held by the caller.
static bool chunkAddLocked(ChunkedList *self) {
    ChunkDir *dir = directoryOf(self);
    uint32_t have = atomic_load(&(*self).chunkCount);
    if (!dir || have >= (*dir).capacity) {
        if (!dirGrow(self))
            return false;
        dir = directoryOf(self);
        if (!dir)
            return false;
        have = atomic_load(&(*self).chunkCount);
        if (have >= (*dir).capacity)
            return false;
    }

    size_t bytes = (size_t)(*self).rowsPerChunk * (size_t)(*self).collection.stride;
    uint64_t chunkType = Type_make(PROJ_VEXSPOKE, FORM_ARRAY, (*self).collection.elementClass);
    uint8_t *chunk = (uint8_t*) Memory_alloc(chunkType, bytes);
    if (!chunk)
        return false;

    memset(chunk, 0, bytes);
    atomic_store_explicit(&(*dir).chunks[have], chunk, memory_order_release);
    atomic_store(&(*self).chunkCount, have + 1u);

    uint64_t cap = ((uint64_t)have + 1u) * (uint64_t)(*self).rowsPerChunk;
    Collection *c = (Collection*) self;
    (*c).capacity = cap > (uint64_t)UINT32_MAX ? UINT32_MAX : (uint32_t)cap;
    return true;
}

// Growth gate body: publish chunks until `needChunks` exist. Gate held.
static bool ensureChunksLocked(ChunkedList *self, uint32_t needChunks) {
    while (atomic_load(&(*self).chunkCount) < needChunks)
        if (!chunkAddLocked(self))
            return false;
    return true;
}

// Growth gate: bounded 100ms try-lock, drop-degrade false on contention or OOM.
static bool ensureChunks(ChunkedList *self, uint32_t needChunks) {
    if (needChunks <= atomic_load(&(*self).chunkCount))
        return true;
    if (!SpinLock_tryLockTimeout(&(*self).lock, VEX_CHUNKED_LOCK_NANOS))
        return false;

    bool ok = ensureChunksLocked(self, needChunks);

    SpinLock_unlock(&(*self).lock);
    return ok;
}

static ChunkedList *instant(uint32_t elementClass, size_t stride, uint32_t chunkBytes) {
    if (stride == 0)
        stride = Stride_get(elementClass);
    if (stride == 0)
        stride = sizeof(void*);
    if (stride > (size_t)UINT32_MAX)
        return nullptr;
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
    atomic_store_explicit(&(*self).directory, nullptr, memory_order_relaxed);
    atomic_store(&(*self).chunkCount, 0u);
    atomic_store(&(*self).committed, 0u);
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
        uint32_t chunks = atomic_load(&(*self).chunkCount);
        for (uint32_t i = 0; i < chunks; i++)
            Memory_free(atomic_load_explicit(&(*dir).chunks[i], memory_order_relaxed));
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
    if (!SpinLock_tryLockTimeout(&(*self).lock, VEX_CHUNKED_LOCK_NANOS))
        return nullptr;

    uint8_t *row = nullptr;
    uint32_t index = atomic_load(&(*self).committed);
    if (index != UINT32_MAX) {
        uint32_t needChunks = (index >> (*self).rowShift) + 1u;
        if (ensureChunksLocked(self, needChunks)) {
            row = rowAt(self, index);
            if (row) {
                Collection *c = (Collection*) self;
                memset(row, 0, (*c).stride);
                atomic_store(&(*self).committed, index + 1u);
                (*c).activeCount = index + 1u;
            }
        }
    }

    SpinLock_unlock(&(*self).lock);
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

    uint32_t rows = atomic_load(&(*self).committed);
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
    if (!self)
        return nullptr;
    if (index >= atomic_load(&(*self).committed))
        return nullptr;
    return rowAt(self, index);
}

uint8_t *ChunkedList_getChunk(const ChunkedList *self, uint32_t chunkIndex) {
    ChunkDir *dir = self ? directoryOf(self) : nullptr;
    if (!dir || chunkIndex >= (*dir).capacity)
        return nullptr;
    return atomic_load_explicit(&(*dir).chunks[chunkIndex], memory_order_acquire);
}

// SETTERS

void ChunkedList_setChunkBytes(ChunkedList *self, uint32_t chunkBytes) {
    if (!self)
        return;
    if (!SpinLock_tryLockTimeout(&(*self).lock, VEX_CHUNKED_LOCK_NANOS))
        return;
    if (atomic_load(&(*self).chunkCount) == 0u) {
        if (chunkBytes < 16u)
            chunkBytes = 16u;
        uint32_t rows = pow2Rows((size_t)(*self).collection.stride, chunkBytes);
        (*self).chunkBytes = chunkBytes;
        (*self).rowsPerChunk = rows;
        (*self).rowShift = shiftOf(rows);
        (*self).rowMask = rows - 1u;
    }
    SpinLock_unlock(&(*self).lock);
}

// GETTERS

uint32_t ChunkedList_size(const ChunkedList *self) {
    if (!self)
        return 0u;
    return atomic_load(&(*self).committed);
}

uint32_t ChunkedList_length(const ChunkedList *self) {
    return ChunkedList_size(self);
}

uint32_t ChunkedList_capacity(const ChunkedList *self) {
    if (!self)
        return 0u;
    uint64_t cap = (uint64_t)atomic_load(&(*self).chunkCount) * (uint64_t)(*self).rowsPerChunk;
    if (cap > (uint64_t)UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)cap;
}

bool ChunkedList_isEmpty(const ChunkedList *self) {
    if (!self)
        return true;
    return atomic_load(&(*self).committed) == 0u;
}

uint32_t ChunkedList_elementClassId(const ChunkedList *self) {
    return self ? (*self).collection.elementClass : 0u;
}

uint32_t ChunkedList_stride(const ChunkedList *self) {
    return self ? (*self).collection.stride : 0u;
}

uint32_t ChunkedList_getChunkCount(const ChunkedList *self) {
    if (!self)
        return 0u;
    return atomic_load(&(*self).chunkCount);
}

uint32_t ChunkedList_getRowsPerChunk(const ChunkedList *self) {
    return self ? (*self).rowsPerChunk : 0u;
}

uint32_t ChunkedList_getChunkBytes(const ChunkedList *self) {
    return self ? (*self).chunkBytes : 0u;
}
