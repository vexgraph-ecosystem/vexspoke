#include "struct/exhaustion.h"

#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: StructExhaustion
 * ============================================================================
 * The one loud-refusal seam for finite-storage owners that do not embed a
 * Collection (MinHeap, SparseSet, Octree, ...). A refusal is counted every time
 * and reported once per epoch: the first refusal emits a THROW naming the owner
 * and the requested size, later ones only increment (the Exhaustion Loudness
 * Law). The owner owns the two state words; this module owns the policy, so the
 * count-and-report rule cannot drift between owners.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: StructExhaustion (struct/exhaustion)
 * ============================================================================
 * PROCEDURAL — no owned class. State lives in the calling owner (a refusal
 * count plus a one-report latch, passed by pointer).
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - Struct_reportExhaustion(count, reported, owner, requestedBytes)
 *   - Struct_exhaustionCount(count)
 *   - Struct_resetExhaustion(count, reported)
 *
 * CONTRACT: count/reported must point at live owner storage. A null pair is a
 * caller defect and is reported rather than silently ignored. Always returns
 * false; the caller still returns its own safe default.
 * ============================================================================
 */

/** Count a refusal and emit the epoch's single diagnostic. Always false. */
;;DEBUG
bool Struct_reportExhaustion(uint64_t *count, bool *reported, const char *owner,
                             size_t requestedBytes, size_t boundBytes) {
    if (count == nullptr || reported == nullptr) {
        THROW("struct exhaustion: null counter (%s, %zu bytes)",
              owner ? owner : "?", requestedBytes);
        return false;
    }
    (*count)++;
    if (!(*reported)) {
        (*reported) = true;
        if (boundBytes != 0)
            THROW("%s: refused, requested %zu bytes (bound %zu)",
                  owner ? owner : "owner", requestedBytes, boundBytes);
        else
            THROW("%s: refused, requested %zu bytes", owner ? owner : "owner", requestedBytes);
    }
    return false;
}

/** Return the owner's refusal count for the current epoch. */
;;TEST
uint64_t Struct_exhaustionCount(const uint64_t *count) {
    return count ? (*count) : 0;
}

/** Start a fresh epoch for the owner. */
;;TEST
void Struct_resetExhaustion(uint64_t *count, bool *reported) {
    if (count) (*count) = 0;
    if (reported) (*reported) = false;
}

// --- Construction refusals (process-wide; no owner instance exists yet) ---

// The epoch's construction accounting. Process-wide because a failed
// construction has no owner instance to carry a counter.
static uint64_t s_constructionCount = 0;
static bool s_constructionReported = false;

/** Count a construction refusal and report the epoch's first one. Always false. */
;;DEBUG
bool Struct_reportConstructionExhaustion(const char *owner, size_t requestedBytes) {
    s_constructionCount++;
    if (!s_constructionReported) {
        s_constructionReported = true;
        THROW("%s: construction refused, could not allocate %zu bytes",
              owner ? owner : "owner", requestedBytes);
    }
    return false;
}

/** Return the process-wide construction-refusal count for the current epoch. */
;;TEST
uint64_t Struct_constructionExhaustionCount(void) {
    return s_constructionCount;
}

/** Start a fresh process-wide construction epoch. */
;;TEST
void Struct_resetConstructionExhaustion(void) {
    s_constructionCount = 0;
    s_constructionReported = false;
}
