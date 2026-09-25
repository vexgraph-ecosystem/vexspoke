#ifndef ALGO_SEGMENT_INDEX_H
#define ALGO_SEGMENT_INDEX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "relational/variable_hash_map.h"
#include "struct/chunked_list.h"

// algo/segment_index.h — positional segment index for dotted-name search.
//
// The Relational Engine's search algorithm. A registered name is one flat
// dotted string (character.position.x); a QUERY is a '.',-split filter, and the
// dot is a splitter, never an address — searching "b.c" matches "a.b.c" without
// knowing the parent, and searching "x" alone is broad on purpose.
//
// The index is positional (the textbook phrase-search shape):
//
//     segment -> postings [ (entry, position) ... ]     "x" -> [(17,2), (23,1)]
//
// A query resolves each segment's postings, walks the RAREST one, and verifies
// the rest contiguously with a binary search — no full scan. Ranking is
// exact (whole name) > suffix (trailing segments) > contains, then by match
// start, then shorter name, then lexicographic — fully deterministic.
//
// COLD PATH ONLY (the Cold-Only Reflection Law): the query is the cold
// rendezvous, and its walk site carries the ;;INTENTION marker. Never a frame.

#define SEGMENT_NAME_BYTES 24u
#define SEGMENT_NAME_MAX 23u
// A valid dotted name of at most 23 chars holds at most ceil(23/2) = 12 segments.
#define SEGMENT_QUERY_SEGMENTS_MAX 12u
#define SEGMENT_ENTRY_NONE UINT32_MAX

// Match tiers (rank order): exact whole-name, trailing suffix, contiguous anywhere.
#define SEGMENT_MATCH_EXACT 0u
#define SEGMENT_MATCH_SUFFIX 1u
#define SEGMENT_MATCH_CONTAINS 2u

// SLOT RECORD (owned by SegmentIndex, behaviorless): one posting.
typedef struct SegmentPosting {
    uint32_t entry;    // entry id
    uint32_t position; // segment position within the entry's name
} SegmentPosting;

// SLOT RECORD (owned by SegmentIndex, behaviorless): one indexed entry.
typedef struct SegmentEntry {
    char name[SEGMENT_NAME_BYTES]; // folded dotted name
    uintptr_t payload;             // caller's value (the reflection search stores the VariableSlot*)
    uint32_t segments;             // segment count
    uint32_t pad;                  // explicit padding
} SegmentEntry;

// One ranked result.
typedef struct SegmentMatch {
    uintptr_t payload; // the entry's payload
    uint32_t entry;    // entry id
    uint32_t start;    // segment position where the match begins
    uint32_t tier;     // SEGMENT_MATCH_*
} SegmentMatch;

typedef struct SegmentIndex {
    bool active;                  // runtime-active flag
    uint32_t count;               // indexed entries (== entry row count)
    ChunkedList *entries;         // never-moved SegmentEntry rows
    VariableHashMap *postings;    // segment -> ChunkedList* of SegmentPosting
} SegmentIndex;

// --- Path splitting (the '.' splitter) ---
// Number of '.'-separated segments in a valid path; 0 when null, empty, has an
// empty segment (leading/trailing/double dot), or a segment over SEGMENT_NAME_MAX.
size_t SegmentIndex_segmentCount(const char *path);
// Copy segment `index` into out (bounded, NUL-terminated). Returns the length,
// or -1 on a null out / short buffer / index past the end.
int SegmentIndex_segment(const char *path, size_t index, char *out, size_t cap);

// --- Constructors ---
bool SegmentIndex_init(SegmentIndex *index);
void SegmentIndex_shutdown(SegmentIndex *index);
SegmentIndex *SegmentIndex_0(void);
void SegmentIndex_free(SegmentIndex *index);
#define SegmentIndex(...) CONSTRUCTOR_DISPATCH(SegmentIndex, __VA_ARGS__)

// --- Core functions ---
// Drop every entry + posting, keeping the index usable.
void SegmentIndex_clear(SegmentIndex *index);
// Index a name (folded + segmented + posted). Returns the entry id, or
// SEGMENT_ENTRY_NONE on an invalid name or OOM.
uint32_t SegmentIndex_add(SegmentIndex *index, const char *name, uintptr_t payload);
// (Re)build the whole index from every live map entry, payload = the VariableSlot*.
// Returns the entry count.
size_t SegmentIndex_buildFrom(SegmentIndex *index, VariableHashMap *map);
// Ranked search (dest-last + truncation flag). Returns the total match count;
// at most cap are written, best first.
size_t SegmentIndex_query(const SegmentIndex *index, const char *query, SegmentMatch *out, size_t cap, bool *outTruncated);

// --- Getters (null-safe) ---
uint32_t SegmentIndex_count(const SegmentIndex *index);
bool SegmentIndex_isEmpty(const SegmentIndex *index);

// --- String projections (the toString Law) ---
void SegmentIndex_toString(const SegmentIndex *index, char *dest, size_t cap, bool *outTruncated);
void SegmentIndex_toStringStruct(const SegmentIndex *index, char *dest, size_t cap, bool *outTruncated);

#endif
