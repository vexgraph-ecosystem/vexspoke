// algo/segment_index.c — positional segment index for dotted-name search.
//
// Postings are the phrase-search shape: segment -> (entry, position). A query
// walks the rarest segment's postings and verifies the rest with a binary
// search. Cold path only.

#include "algo/segment_index.h"

#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "relational/variable_slot.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: SegmentIndex
 * ============================================================================
 * The Relational Engine's dotted-name search. A registered name is one flat
 * dotted string; a query is a '.',-split filter. The index maps each segment to
 * a positional posting list (entry, position) in a VariableHashMap whose slot
 * lists are ChunkedLists, so building is one pass over the names and a query is
 * "resolve each segment's list, walk the rarest, verify the rest contiguously".
 *
 * Ranking: exact (whole name) > suffix (trailing segments) > contiguous; ties by
 * match start, then shorter name, then lexicographic — deterministic.
 *
 * Path temperature: cold. The query walk is the cold rendezvous and carries the
 * ;;INTENTION marker (the Cold-Only Reflection Law); it must never run on a
 * frame. All getters are null-safe.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: SegmentIndex (algo/segment_index.c)
 * LEVEL: L2 — Behavior (search algorithm)
 * ============================================================================
 * a positional segment index + ranked phrase search over dotted names.
 *
 * STRUCT FIELDS (Mirroring algo/segment_index.h):
 * ----------------------------------------------------------------------------
 *   SegmentIndex {
 *     bool active;               // runtime-active flag
 *     uint32_t count;            // indexed entries (== entry row count)
 *     ChunkedList *entries;      // never-moved SegmentEntry rows
 *     VariableHashMap *postings; // segment -> ChunkedList* of SegmentPosting
 *   }
 *
 * SLOT RECORDS (owned by SegmentIndex, behaviorless):
 * ----------------------------------------------------------------------------
 *   SegmentPosting entry;    // uint32_t + the entry id
 *   SegmentPosting position; // uint32_t + the segment position
 *   SegmentEntry name;       // char[24] + the folded dotted name
 *   SegmentEntry payload;    // uintptr_t + the caller's value
 *   SegmentEntry segments;   // uint32_t + the segment count
 *   SegmentEntry pad;        // uint32_t + explicit row padding
 *
 * PRIVATE HELPERS (kept file-local, pure logic, no behavior of their own): none.
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - SegmentIndex_0()                     : arena-allocated empty index
 *   - SegmentIndex_init(index)             : embedded init
 *   - SegmentIndex_free(index)             : shutdown + release an arena index
 *   - SegmentIndex_shutdown(index)         : free posting lists + entries
 *   - SegmentIndex_clear(index)            : drop entries + postings, stay usable
 *
 * Private Core Functions: (.c static)
 *   - entryAt(index, entry)                : bounds-checked entry resolve
 *   - postSegment(index, entry, pos, seg)  : append one posting
 *   - findPosting(list, entry, position)   : binary search inside a posting list
 *   - splitFold(path, segs)                : fold + split a query into segments
 *   - matchBefore(index, a, b)             : the ranking comparator
 *   - insertMatch(index, out, cap, kept, m): top-cap insertion sort
 *   - buildFn(slot, userdata)              : map forEach -> add
 *   - freePostingsFn(slot, userdata)       : map forEach -> free a posting list
 *
 * Public Core Functions: (.h)
 *   - SegmentIndex_segmentCount(path)
 *   - SegmentIndex_segment(path, index, out, cap)
 *   - SegmentIndex_add(index, name, payload)
 *   - SegmentIndex_buildFrom(index, map)
 *   - SegmentIndex_query(index, query, out, cap, outTruncated)
 *
 * Public Getters: (.h)
 *   - SegmentIndex_count(index)
 *   - SegmentIndex_isEmpty(index)
 *
 * Public String Projections: (.h)
 *   - SegmentIndex_toString(index, dest, cap, outTruncated)
 *   - SegmentIndex_toStringStruct(index, dest, cap, outTruncated)
 * ============================================================================
 */

// PRIVATE HELPERS

static const SegmentEntry *entryAt(const SegmentIndex *index, uint32_t entry) {
    if (!index || !(*index).entries)
        return nullptr;
    return (const SegmentEntry*) ChunkedList_slot((*index).entries, entry);
}

// Append one posting (entry, position) for `segment`, minting the segment's
// list on first use. Postings stay sorted by entry because entries are added in
// order, so findPosting can binary search.
static bool postSegment(SegmentIndex *index, uint32_t entry, uint32_t position, const char *segment) {
    VariableHashMap *postings = (*index).postings;
    uintptr_t existing = 0u;
    ChunkedList *list = nullptr;
    if (VariableHashMap_get(postings, segment, &existing))
        list = (ChunkedList*) existing;
    if (!list) {
        list = ChunkedList_3(ID_SEGMENT_POSTING, (uint32_t) sizeof(SegmentPosting), VEX_CHUNKED_BYTES_DEFAULT);
        if (!list)
            return false;
        if (!VariableHashMap_add(postings, segment, (uintptr_t) list)) {
            ChunkedList_free(list);
            return false;
        }
    }
    SegmentPosting *row = (SegmentPosting*) ChunkedList_addSlot(list);
    if (!row)
        return false;
    (*row).entry = entry;
    (*row).position = position;
    return true;
}

// Binary search by entry, then scan the equal-entry run for the position.
static bool findPosting(const ChunkedList *list, uint32_t entry, uint32_t position) {
    if (!list)
        return false;
    uint32_t lo = 0u;
    uint32_t hi = ChunkedList_size(list);
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2u;
        const SegmentPosting *p = (const SegmentPosting*) ChunkedList_slot(list, mid);
        if (!p)
            return false;
        if ((*p).entry < entry)
            lo = mid + 1u;
        else
            hi = mid;
    }
    uint32_t n = ChunkedList_size(list);
    for (uint32_t i = lo; i < n; i++) {
        const SegmentPosting *p = (const SegmentPosting*) ChunkedList_slot(list, i);
        if (!p || (*p).entry != entry)
            return false;
        if ((*p).position == position)
            return true;
    }
    return false;
}

// Fold a path and split it into up to SEGMENT_QUERY_SEGMENTS_MAX segments.
static size_t splitFold(const char *path, char segs[][SEGMENT_NAME_BYTES]) {
    char folded[SEGMENT_NAME_BYTES];
    if (!VariableSlot_foldName(path, folded))
        return 0u;
    size_t n = 0u;
    const char *p = folded;
    while (*p != '\0' && n < SEGMENT_QUERY_SEGMENTS_MAX) {
        const char *dot = strchr(p, '.');
        size_t len = dot ? (size_t)(dot - p) : strlen(p);
        if (len == 0u || len >= SEGMENT_NAME_BYTES)
            return 0u;
        memcpy(segs[n], p, len);
        segs[n][len] = '\0';
        n++;
        if (!dot)
            break;
        p = dot + 1;
    }
    return n;
}

// Ranking: tier, then match start, then shorter name, then lexicographic.
static bool matchBefore(const SegmentIndex *index, const SegmentMatch *a, const SegmentMatch *b) {
    if ((*a).tier != (*b).tier)
        return (*a).tier < (*b).tier;
    if ((*a).start != (*b).start)
        return (*a).start < (*b).start;
    const SegmentEntry *ea = entryAt(index, (*a).entry);
    const SegmentEntry *eb = entryAt(index, (*b).entry);
    if (!ea || !eb)
        return ea != nullptr;
    size_t la = strlen((*ea).name);
    size_t lb = strlen((*eb).name);
    if (la != lb)
        return la < lb;
    return strcmp((*ea).name, (*eb).name) < 0;
}

// Insert into the bounded best-first output array (top-`cap`, insertion sort).
static void insertMatch(const SegmentIndex *index, SegmentMatch *out, size_t cap, size_t *kept, const SegmentMatch *m) {
    size_t pos = 0u;
    while (pos < *kept && !matchBefore(index, m, &out[pos]))
        pos++;
    if (pos >= cap)
        return;
    size_t size = *kept;
    if (size < cap) {
        size++;
        *kept = size;
    }
    for (size_t i = size - 1u; i > pos; i--)
        out[i] = out[i - 1u];
    out[pos] = *m;
}

typedef struct BuildCtx {
    SegmentIndex *index;
} BuildCtx;

static void buildFn(VariableSlot *slot, void *userdata) {
    BuildCtx *ctx = (BuildCtx*) userdata;
    char name[SEGMENT_NAME_BYTES];
    if (VariableSlot_getName(slot, name, sizeof(name)) < 0)
        return;
    SegmentIndex_add((*ctx).index, name, (uintptr_t) slot);
}

static void freePostingsFn(VariableSlot *slot, void *userdata) {
    (void) userdata;
    uintptr_t value = VariableSlot_getPointer(slot);
    if (value)
        ChunkedList_free((ChunkedList*) value);
}

// PATH SPLITTING

size_t SegmentIndex_segmentCount(const char *path) {
    if (!path || path[0] == '\0')
        return 0u;
    size_t count = 0u;
    size_t len = 0u;
    for (const char *p = path; ; p++) {
        char c = *p;
        if (c != '\0' && c != '.') {
            len++;
            if (len > SEGMENT_NAME_MAX)
                return 0u;
            continue;
        }
        if (len == 0u)
            return 0u;
        count++;
        len = 0u;
        if (c == '\0')
            return count;
    }
}

int SegmentIndex_segment(const char *path, size_t index, char *out, size_t cap) {
    if (!path || !out || cap == 0u)
        return -1;
    if (SegmentIndex_segmentCount(path) <= index)
        return -1;
    size_t seg = 0u;
    const char *start = path;
    const char *p = path;
    for (; ; p++) {
        char c = *p;
        if (c != '\0' && c != '.')
            continue;
        size_t len = (size_t)(p - start);
        if (len > 0u) {
            if (seg == index) {
                if (len + 1u > cap)
                    return -1;
                memcpy(out, start, len);
                out[len] = '\0';
                return (int) len;
            }
            seg++;
        }
        if (c == '\0')
            return -1;
        start = p + 1;
    }
}

// CONSTRUCTORS

bool SegmentIndex_init(SegmentIndex *index) {
    if (!index)
        return false;
    memset(index, 0, sizeof(*index));
    ChunkedList *entries = ChunkedList_3(ID_SEGMENT_ENTRY, (uint32_t) sizeof(SegmentEntry), VEX_CHUNKED_BYTES_DEFAULT);
    if (!entries)
        return false;
    VariableHashMap *postings = VariableHashMap_0();
    if (!postings) {
        ChunkedList_free(entries);
        return false;
    }
    (*index).active = true;
    (*index).count = 0u;
    (*index).entries = entries;
    (*index).postings = postings;
    return true;
}

void SegmentIndex_shutdown(SegmentIndex *index) {
    if (!index)
        return;
    if ((*index).postings) {
        VariableHashMap_forEach((*index).postings, freePostingsFn, nullptr);
        VariableHashMap_free((*index).postings);
    }
    if ((*index).entries)
        ChunkedList_free((*index).entries);
    (*index).postings = nullptr;
    (*index).entries = nullptr;
    (*index).count = 0u;
    (*index).active = false;
}

SegmentIndex *SegmentIndex_0(void) {
    SegmentIndex *index = (SegmentIndex*) Memory_alloc(TYPE_SEGMENT_INDEX, sizeof(SegmentIndex));
    if (!index)
        return nullptr;
    if (!SegmentIndex_init(index)) {
        Memory_free(index);
        return nullptr;
    }
    return index;
}

void SegmentIndex_free(SegmentIndex *index) {
    if (!index)
        return;
    SegmentIndex_shutdown(index);
    Memory_free(index);
}

// CORE FUNCTIONS

void SegmentIndex_clear(SegmentIndex *index) {
    if (!index || !(*index).active)
        return;
    VariableHashMap_forEach((*index).postings, freePostingsFn, nullptr);
    VariableHashMap_shutdown((*index).postings);
    VariableHashMap_init((*index).postings);
    ChunkedList_free((*index).entries);
    ChunkedList *entries = ChunkedList_3(ID_SEGMENT_ENTRY, (uint32_t) sizeof(SegmentEntry), VEX_CHUNKED_BYTES_DEFAULT);
    (*index).entries = entries;
    (*index).count = 0u;
    if (!entries)
        (*index).active = false;
}

uint32_t SegmentIndex_add(SegmentIndex *index, const char *name, uintptr_t payload) {
    if (!index || !(*index).active)
        return SEGMENT_ENTRY_NONE;
    char folded[SEGMENT_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return SEGMENT_ENTRY_NONE;
    SegmentEntry *row = (SegmentEntry*) ChunkedList_addSlot((*index).entries);
    if (!row)
        return SEGMENT_ENTRY_NONE;
    memset(row, 0, sizeof(*row));
    memcpy((*row).name, folded, strlen(folded) + 1);
    (*row).payload = payload;
    uint32_t entry = ChunkedList_size((*index).entries) - 1u;

    uint32_t position = 0u;
    const char *p = folded;
    while (*p != '\0') {
        const char *dot = strchr(p, '.');
        size_t len = dot ? (size_t)(dot - p) : strlen(p);
        char segment[SEGMENT_NAME_BYTES];
        memcpy(segment, p, len);
        segment[len] = '\0';
        if (!postSegment(index, entry, position, segment)) {
            (*row).segments = position;
            (*index).count = entry + 1u;
            return SEGMENT_ENTRY_NONE;
        }
        position++;
        if (!dot)
            break;
        p = dot + 1;
    }
    (*row).segments = position;
    (*index).count = entry + 1u;
    return entry;
}

size_t SegmentIndex_buildFrom(SegmentIndex *index, VariableHashMap *map) {
    if (!index || !map)
        return 0u;
    SegmentIndex_clear(index);
    BuildCtx ctx = { index };
    VariableHashMap_forEach(map, buildFn, &ctx);
    return SegmentIndex_count(index);
}

size_t SegmentIndex_query(const SegmentIndex *index, const char *query, SegmentMatch *out, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!index || !(*index).active || !query || !out || cap == 0u)
        return 0u;

    char segs[SEGMENT_QUERY_SEGMENTS_MAX][SEGMENT_NAME_BYTES];
    size_t qn = splitFold(query, segs);
    if (qn == 0u)
        return 0u;

    const ChunkedList *lists[SEGMENT_QUERY_SEGMENTS_MAX];
    size_t rarest = 0u;
    for (size_t k = 0u; k < qn; k++) {
        uintptr_t value = 0u;
        if (!VariableHashMap_get((*index).postings, segs[k], &value) || value == 0u)
            return 0u;
        lists[k] = (const ChunkedList*) value;
        if (ChunkedList_size(lists[k]) < ChunkedList_size(lists[rarest]))
            rarest = k;
    }

    ;;INTENTION("cold path search is a node walk by design; hot iteration stays DOD")
    const ChunkedList *cand = lists[rarest];
    uint32_t candCount = ChunkedList_size(cand);
    size_t kept = 0u;
    size_t total = 0u;
    for (uint32_t i = 0u; i < candCount; i++) {
        const SegmentPosting *p = (const SegmentPosting*) ChunkedList_slot(cand, i);
        if (!p || (*p).position < rarest)
            continue;
        uint32_t entry = (*p).entry;
        uint32_t start = (*p).position - (uint32_t) rarest;

        bool ok = true;
        for (size_t k = 0u; k < qn; k++) {
            if (!findPosting(lists[k], entry, start + (uint32_t) k)) {
                ok = false;
                break;
            }
        }
        if (!ok)
            continue;

        const SegmentEntry *e = entryAt(index, entry);
        if (!e)
            continue;
        uint32_t entrySegments = (*e).segments;
        uint32_t tier = SEGMENT_MATCH_CONTAINS;
        if (qn == entrySegments && start == 0u)
            tier = SEGMENT_MATCH_EXACT;
        else if (start + (uint32_t) qn == entrySegments)
            tier = SEGMENT_MATCH_SUFFIX;

        SegmentMatch m;
        m.payload = (*e).payload;
        m.entry = entry;
        m.start = start;
        m.tier = tier;
        insertMatch(index, out, cap, &kept, &m);
        total++;
    }

    if (outTruncated)
        *outTruncated = total > kept;
    return total;
}

// GETTERS

uint32_t SegmentIndex_count(const SegmentIndex *index) {
    return index ? (*index).count : 0u;
}

bool SegmentIndex_isEmpty(const SegmentIndex *index) {
    if (!index)
        return true;
    return (*index).count == 0u;
}

// STRING PROJECTIONS (the toString Law)

void SegmentIndex_toString(const SegmentIndex *index, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!dest || cap == 0u)
        return;
    if (!index) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "SegmentIndex(entries=%u)", (*index).count);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}

void SegmentIndex_toStringStruct(const SegmentIndex *index, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!dest || cap == 0u)
        return;
    if (!index) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "SegmentIndex { active=%s, count=%u, entries=0x%llx, postings=0x%llx }",
                           (*index).active ? "true" : "false", (*index).count,
                           (unsigned long long) (uintptr_t) (*index).entries,
                           (unsigned long long) (uintptr_t) (*index).postings);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}
