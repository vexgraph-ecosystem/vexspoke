// struct/chunked_list.c — never-moved radix-paged list implementation.
//
// A growable copy-on-write root, any number of fixed-radix internal levels, and
// a page-sized leaf. Every hop is shift/mask; a leaf is a contiguous run of
// rows. The 2-level byte-budget form is the degenerate case (root -> leaf).

#include "struct/chunked_list.h"

#include <stdatomic.h>
#include <string.h>

#include "atomic/spin.h"
#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/stride.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ChunkedList
 * ============================================================================
 * Never-moved radix-paged list: rows live in stable leaves reached through a
 * page table (a growable COW root, fixed-radix internal nodes, a page-sized
 * leaf), so a row address handed out once stays valid until free — the sibling
 * struct/List grows one contiguous buffer and therefore moves every element on
 * growth. Radices are powers of two, so every hop is shift/mask with no divide.
 *
 * The root is copy-on-write (old generations stay mapped and valid for racing
 * readers); internal nodes and leaves are published with an acquire/release
 * handoff and never move; the gate serializes claim-plus-growth-plus-commit with
 * a bounded 100ms try-lock per the Bounded Wait Law. Levels default to 2 (root
 * -> leaf) for the byte-budget constructors; the radix form builds any depth.
 * Lives at R2 as a leaf container behavior.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ChunkedList (struct/chunked_list.c)
 * LEVEL: L2 — Behavior (container behavior API)
 * ============================================================================
 * Never-moved radix-paged list: rows live in stable leaves, so a row address
 * handed out once stays valid until free.
 *
 * Concurrency: addSlot and reserve are safe from any number of writers — the
 * gate serializes the whole claim-plus-growth-plus-commit step, so every row is
 * handed out exactly once. slot, getChunk, the count getters and packInto are
 * lock-free and safe concurrently with writers: the committed count is stored
 * only after the row's leaf is published, so index < committed implies the row
 * resolves. setChunkBytes is setup-time only and free requires quiescence.
 *
 * Storage: radices[d] is the fan-out of a node at depth d (powers of two), and
 * the leaf radix is radices[levels-1] = rowsPerChunk. shifts[d] is the bit
 * offset of depth d's digit; masks[d] = radices[d]-1 (masks[0] is unused — the
 * root is growable). Lookup is digit = (index >> shifts[d]) & masks[d] with no
 * divide. The root generation is copy-on-write; internal nodes and leaves are
 * atomic pointers published only after initialization.
 *
 * STRUCT FIELDS (Mirroring struct/chunked_list.h):
 * ----------------------------------------------------------------------------
 *   ChunkedList {
 *     Collection collection;     // gate-maintained mirror; quiesced reads only under concurrency
 *     _Atomic(void*) directory;  // current root generation (COW; generations never freed mid-run)
 *     _Atomic uint32_t chunkCount; // published leaves (lock-free reader bound)
 *     _Atomic uint32_t committed;  // published rows (lock-free reader bound)
 *     uint32_t rowsPerChunk;     // rows per leaf (power of two, >= 1; geometry)
 *     uint32_t rowShift;         // log2(rowsPerChunk)
 *     uint32_t rowMask;          // rowsPerChunk - 1
 *     uint32_t chunkBytes;       // byte budget of one leaf (>= 16)
 *     uint32_t levels;           // pointer hops root..leaf (>= 2)
 *     uint32_t *radices;         // [levels] fan-out per depth; [0] = initial root slots
 *     uint32_t *shifts;          // [levels] bit offset of each depth's digit
 *     uint32_t *masks;           // [levels] radix-1 per depth
 *     SpinLock lock;             // claim + growth + commit; try-locked, 100ms bounded
 *   }
 *
 * PRIVATE HELPERS (kept file-local, pure data + growth math only):
 * ----------------------------------------------------------------------------
 *   ChunkDir {                // one root generation (COW snapshot unit)
 *     struct ChunkDir *prev;  // previous generation, freed at teardown
 *     uint32_t capacity;      // depth-1 slots in THIS generation
 *     uint32_t pad;           // explicit padding so slots[] is 8-byte aligned
 *     _Atomic(uint8_t*) slots[]; // depth-1 objects (nodes or leaves)
 *   }
 *   pow2Rows(stride, chunkBytes)      // largest power-of-two rows for a budget
 *   pow2Floor(n)                      // largest power of two <= n (min 1)
 *   shiftOf(rows)                     // log2 of a power-of-two row count
 *   pageLeafRows(stride)              // default page-sized leaf (pow2 <= 4096/stride)
 *   directoryOf(self)                 // lock-free root generation load
 *   digitOf(self, index, depth)       // shift/mask digit at a depth
 *   leafAtRow(self, index)            // lock-free leaf resolve by row index
 *   rowAt(self, index)                // lock-free stable row resolve
 *   nodeAlloc(self, depth)            // allocate + zero one node or leaf
 *   dirGrow(self)                     // COW root doubling (gate held, cold)
 *   ensureRootSlotLocked(self, slot)  // grow the root until slot fits (gate held)
 *   chunkAddLocked(self)              // create + publish one leaf (gate held, cold)
 *   ensureChunksLocked(self, need)    // growth gate body (gate held, cold)
 *   ensureChunks(self, need)          // bounded-lock growth gate
 *   buildPaged(elementClass, stride, radices, levels, chunkBytes) // shared builder
 *   freeSubtree(node, self, depth)    // recursive teardown free
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - ChunkedList(elementClass, stride, r0, ..., leafRows) : radix form
 *   - ChunkedList_paged(elementClass, stride, radices, count)
 *   - ChunkedList_1(elementClass)                    : ChunkedList_1(elementClass)
 *   - ChunkedList_2(elementClass, chunkBytes)        : ChunkedList_2(elementClass, chunkBytes)
 *   - ChunkedList_3(elementClass, stride, chunkBytes): 2-level byte-budget form
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
 *   - ChunkedList_getLevels(self)
 * ============================================================================
 */

// Growth gate bound (the Bounded Wait Law): 100ms is ~6 frames of slack and
// only expires when another thread is wedged, never on a healthy path.
#define VEX_CHUNKED_LOCK_NANOS 100000000ll

// PRIVATE HELPERS

typedef struct ChunkDir {
    struct ChunkDir *prev; // previous generation (freed last at teardown)
    uint32_t capacity;     // depth-1 slots in THIS generation
    uint32_t pad;          // explicit padding so slots[] is 8-byte aligned
    _Atomic(uint8_t*) slots[]; // depth-1 objects (internal nodes or leaves)
} ChunkDir;

// Largest power of two rows that fits a byte budget (min 1). The loop test
// divides instead of shifting so it cannot wrap on large budgets.
static uint32_t pow2Rows(size_t stride, uint32_t chunkBytes) {
    if (stride == 0u)
        return 1u;
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

// Largest power of two <= n (min 1).
static uint32_t pow2Floor(uint32_t n) {
    if (n <= 1u)
        return 1u;
    uint32_t p = 1u;
    while (p <= (n >> 1))
        p <<= 1u;
    return p;
}

// Default radix-form leaf: one page of rows.
static uint32_t pageLeafRows(size_t stride) {
    if (stride == 0u)
        stride = 1u;
    return pow2Floor((uint32_t)(VEX_CHUNKED_PAGE_BYTES / stride));
}

static ChunkDir *directoryOf(const ChunkedList *self) {
    return (ChunkDir*) atomic_load_explicit(&(*self).directory, memory_order_acquire);
}

// Shift/mask digit at a depth. Depth 0 (the root) is unmasked: it is growable.
static inline uint32_t digitOf(const ChunkedList *self, uint32_t index, uint32_t depth) {
    uint32_t v = index >> (*self).shifts[depth];
    return depth == 0u ? v : (v & (*self).masks[depth]);
}

// Lock-free leaf resolve by row index (no offset): null when not published yet.
static uint8_t *leafAtRow(const ChunkedList *self, uint32_t index) {
    ChunkDir *dir = directoryOf(self);
    if (!dir)
        return nullptr;
    uint32_t d0 = digitOf(self, index, 0u);
    if (d0 >= (*dir).capacity)
        return nullptr;
    uint8_t *node = atomic_load_explicit(&(*dir).slots[d0], memory_order_acquire);
    if (!node)
        return nullptr;
    uint32_t levels = (*self).levels;
    for (uint32_t depth = 1u; depth < levels - 1u; depth++) {
        _Atomic(uint8_t*) *children = (_Atomic(uint8_t*) *) node;
        uint32_t digit = digitOf(self, index, depth);
        node = atomic_load_explicit(&children[digit], memory_order_acquire);
        if (!node)
            return nullptr;
    }
    return node;
}

// Lock-free stable row resolve: null when the row is not published yet.
// Callers bound index by the committed count first; the committed store lands
// only after the leaf publish, so index < committed implies non-null here.
static uint8_t *rowAt(const ChunkedList *self, uint32_t index) {
    uint8_t *leaf = leafAtRow(self, index);
    if (!leaf)
        return nullptr;
    size_t stride = (*self).collection.stride;
    return leaf + (size_t)(index & (*self).rowMask) * stride;
}

// Allocate + zero one node at `depth`: an internal pointer array, or a leaf of
// rows at the deepest depth.
static uint8_t *nodeAlloc(const ChunkedList *self, uint32_t depth) {
    uint32_t radix = (*self).radices[depth];
    if (depth == (*self).levels - 1u) {
        size_t bytes = (size_t)radix * (size_t)(*self).collection.stride;
        uint64_t chunkType = Type_make(PROJ_VEXSPOKE, FORM_ARRAY, (*self).collection.elementClass);
        uint8_t *leaf = (uint8_t*) Memory_alloc(chunkType, bytes);
        if (leaf)
            memset(leaf, 0, bytes);
        return leaf;
    }
    size_t bytes = (size_t)radix * sizeof(_Atomic(uint8_t*));
    uint8_t *node = (uint8_t*) Memory_alloc(TYPE_CHUNKED_LIST, bytes);
    if (node)
        memset(node, 0, bytes);
    return node;
}

// Copy-on-write root doubling: publish a bigger generation, keep the old one
// valid forever (the arraycopy of the cold path — node pointers only). Gate
// held by the caller: exactly one grower, so chunkCount only moves here.
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
    memset(&(*next).slots[0], 0, (size_t)newCap * sizeof(_Atomic(uint8_t*)));

    uint32_t have = old ? (*old).capacity : 0u;
    for (uint32_t i = 0; i < have; i++)
        atomic_store_explicit(&(*next).slots[i], atomic_load_explicit(&(*old).slots[i], memory_order_relaxed), memory_order_relaxed);

    atomic_store_explicit(&(*self).directory, next, memory_order_release);
    return true;
}

// Grow the root until depth-1 slot `slot` exists. Gate held.
static bool ensureRootSlotLocked(ChunkedList *self, uint32_t slot) {
    ChunkDir *dir = directoryOf(self);
    while (!dir || slot >= (*dir).capacity) {
        if (!dirGrow(self))
            return false;
        dir = directoryOf(self);
        if (!dir)
            return false;
    }
    return true;
}

// Create + publish one leaf, walking (and creating) its internal path. Gate
// held by the caller. Rows are zeroed before publish (acquire/release handoff).
static bool chunkAddLocked(ChunkedList *self) {
    uint32_t leafIndex = atomic_load(&(*self).chunkCount);
    if (leafIndex == UINT32_MAX)
        return false;
    uint32_t index = leafIndex << (*self).rowShift;
    uint32_t levels = (*self).levels;

    uint32_t d0 = digitOf(self, index, 0u);
    if (!ensureRootSlotLocked(self, d0))
        return false;
    ChunkDir *dir = directoryOf(self);
    if (!dir)
        return false;

    _Atomic(uint8_t*) *slot = &(*dir).slots[d0];
    for (uint32_t depth = 1u; depth < levels; depth++) {
        if (depth == levels - 1u) {
            uint8_t *leaf = nodeAlloc(self, depth);
            if (!leaf)
                return false;
            atomic_store_explicit(slot, leaf, memory_order_release);
            break;
        }
        uint8_t *node = atomic_load_explicit(slot, memory_order_acquire);
        if (!node) {
            node = nodeAlloc(self, depth);
            if (!node)
                return false;
            atomic_store_explicit(slot, node, memory_order_release);
        }
        _Atomic(uint8_t*) *children = (_Atomic(uint8_t*) *) node;
        uint32_t digit = digitOf(self, index, depth);
        slot = &children[digit];
    }

    atomic_store(&(*self).chunkCount, leafIndex + 1u);
    uint64_t cap = ((uint64_t)leafIndex + 1u) * (uint64_t)(*self).rowsPerChunk;
    Collection *c = (Collection*) self;
    (*c).capacity = cap > (uint64_t)UINT32_MAX ? UINT32_MAX : (uint32_t)cap;
    return true;
}

// Growth gate body: publish leaves until `needChunks` exist. Gate held.
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

// Recompute the shift/mask tables from radices (geometry change, gate held).
static void recomputeGeometry(ChunkedList *self) {
    uint32_t levels = (*self).levels;
    uint32_t *radices = (*self).radices;
    uint32_t *shifts = (*self).shifts;
    uint32_t *masks = (*self).masks;
    uint32_t rows = radices[levels - 1u];
    (*self).rowsPerChunk = rows;
    (*self).rowShift = shiftOf(rows);
    (*self).rowMask = rows - 1u;
    shifts[levels - 1u] = 0u;
    masks[levels - 1u] = rows - 1u;
    for (uint32_t d = levels - 1u; d > 0u; d--) {
        shifts[d - 1u] = shifts[d] + shiftOf(radices[d]);
        masks[d - 1u] = (d - 1u) == 0u ? 0u : (radices[d - 1u] - 1u);
    }
}

// Shared builder: allocate the struct + geometry tables, zero the counters.
// `chunkBytes` 0 derives the leaf byte size from rows*stride.
static ChunkedList *buildPaged(uint32_t elementClass, size_t stride, const uint32_t *radixIn, uint32_t levels, uint32_t chunkBytes) {
    if (levels < 2u)
        levels = 2u;
    if (stride == 0u)
        stride = Stride_get(elementClass);
    if (stride == 0u)
        stride = sizeof(void*);
    if (stride > (size_t)UINT32_MAX)
        return nullptr;

    ChunkedList *self = (ChunkedList*) Memory_alloc(TYPE_CHUNKED_LIST, sizeof(ChunkedList));
    if (!self)
        return nullptr;
    memset(self, 0, sizeof(*self));

    uint32_t *radices = (uint32_t*) Memory_alloc(TYPE_CHUNKED_LIST, (size_t)levels * sizeof(uint32_t));
    uint32_t *shifts = (uint32_t*) Memory_alloc(TYPE_CHUNKED_LIST, (size_t)levels * sizeof(uint32_t));
    uint32_t *masks = (uint32_t*) Memory_alloc(TYPE_CHUNKED_LIST, (size_t)levels * sizeof(uint32_t));
    if (!radices || !shifts || !masks) {
        Memory_free(radices);
        Memory_free(shifts);
        Memory_free(masks);
        Memory_free(self);
        return nullptr;
    }
    for (uint32_t d = 0u; d < levels; d++) {
        uint32_t r = radixIn[d];
        radices[d] = r == 0u ? 1u : pow2Floor(r);
    }
    (*self).levels = levels;
    (*self).radices = radices;
    (*self).shifts = shifts;
    (*self).masks = masks;
    recomputeGeometry(self);

    if (chunkBytes == 0u) {
        uint64_t b = (uint64_t)radices[levels - 1u] * (uint64_t)stride;
        chunkBytes = b > (uint64_t)UINT32_MAX ? UINT32_MAX : (uint32_t)b;
    }
    if (chunkBytes < 16u)
        chunkBytes = 16u;
    (*self).chunkBytes = chunkBytes;

    Collection *c = (Collection*) self;
    (*c).typeId = TYPE_CHUNKED_LIST;
    (*c).activeCount = 0;
    (*c).elementClass = elementClass;
    (*c).stride = (uint32_t)stride;
    (*c).capacity = 0;
    (*c).head = 0;
    (*c).data = nullptr;

    atomic_store_explicit(&(*self).directory, nullptr, memory_order_relaxed);
    atomic_store(&(*self).chunkCount, 0u);
    atomic_store(&(*self).committed, 0u);
    (*self).lock = SPIN_LOCK_INIT;
    return self;
}

// The 2-level byte-budget form (the historical shape).
static ChunkedList *instant(uint32_t elementClass, size_t stride, uint32_t chunkBytes) {
    if (stride == 0u)
        stride = Stride_get(elementClass);
    if (stride == 0u)
        stride = sizeof(void*);
    if (stride > (size_t)UINT32_MAX)
        return nullptr;
    if (chunkBytes < 16u)
        chunkBytes = 16u;
    uint32_t rows = pow2Rows(stride, chunkBytes);
    uint32_t radixIn[2] = { VEX_CHUNKED_DIR_INIT, rows };
    return buildPaged(elementClass, stride, radixIn, 2u, chunkBytes);
}

// Recursive teardown: free a depth-`depth` object and everything under it.
static void freeSubtree(uint8_t *node, const ChunkedList *self, uint32_t depth) {
    if (!node)
        return;
    if (depth == (*self).levels - 1u) {
        Memory_free(node);
        return;
    }
    _Atomic(uint8_t*) *children = (_Atomic(uint8_t*) *) node;
    uint32_t radix = (*self).radices[depth];
    for (uint32_t i = 0u; i < radix; i++)
        freeSubtree(atomic_load_explicit(&children[i], memory_order_relaxed), self, depth + 1u);
    Memory_free(node);
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

ChunkedList *ChunkedList_paged(uint32_t elementClass, uint32_t stride, const int32_t *radices, size_t count) {
    if (stride == 0u)
        stride = (uint32_t) Stride_get(elementClass);
    if (stride == 0u)
        stride = (uint32_t) sizeof(void*);

    size_t cap = count > 3u ? count : 3u;
    uint32_t *shape = (uint32_t*) Memory_alloc(TYPE_CHUNKED_LIST, cap * sizeof(uint32_t));
    if (!shape)
        return nullptr;

    uint32_t levels = 2u;
    bool preset = count >= 1u && radices[0] < 0;
    if (preset) {
        // A *_LAYER_DEFAULT sentinel selects the shape; explicit numbers after
        // it override the corresponding level (index 1 upward).
        if (radices[0] == CHUNKED_LIST_THREE_LAYER_DEFAULT)
            levels = 3u;
        else
            levels = 2u;
        if (levels > cap) {
            Memory_free(shape);
            return nullptr;
        }
        shape[0] = VEX_CHUNKED_DIR_INIT;
        for (uint32_t d = 1u; d + 1u < levels; d++)
            shape[d] = VEX_CHUNKED_INTERNAL_RADIX_DEFAULT;
        shape[levels - 1u] = pageLeafRows(stride);
        for (size_t k = 1u; k < count; k++) {
            if (radices[k] > 0 && k < (size_t)levels)
                shape[k] = (uint32_t) radices[k];
        }
    } else {
        levels = count < 2u ? 2u : (uint32_t) count;
        if (levels > cap) {
            Memory_free(shape);
            return nullptr;
        }
        for (uint32_t d = 0u; d < levels; d++) {
            int32_t v = d < (uint32_t) count ? radices[d] : 0;
            if (v > 0)
                shape[d] = (uint32_t) v;
            else
                shape[d] = d == 0u ? VEX_CHUNKED_DIR_INIT : 1u;
        }
    }

    ChunkedList *self = buildPaged(elementClass, stride, shape, levels, 0u);
    Memory_free(shape);
    return self;
}

// CORE FUNCTIONS

void ChunkedList_free(ChunkedList *self) {
    if (!self)
        return;
    ChunkDir *dir = directoryOf(self);
    if (dir) {
        for (uint32_t i = 0u; i < (*dir).capacity; i++)
            freeSubtree(atomic_load_explicit(&(*dir).slots[i], memory_order_relaxed), self, 1u);
    }
    while (dir) {
        ChunkDir *prev = (*dir).prev;
        Memory_free(dir);
        dir = prev;
    }
    Memory_free((*self).radices);
    Memory_free((*self).shifts);
    Memory_free((*self).masks);
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
    if (!self)
        return nullptr;
    if (chunkIndex > (UINT32_MAX >> (*self).rowShift))
        return nullptr;
    return leafAtRow(self, chunkIndex << (*self).rowShift);
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
        (*self).radices[(*self).levels - 1u] = rows;
        recomputeGeometry(self);
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

uint32_t ChunkedList_getLevels(const ChunkedList *self) {
    return self ? (*self).levels : 0u;
}
