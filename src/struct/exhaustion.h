#ifndef STRUCT_EXHAUSTION_H
#define STRUCT_EXHAUSTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "annotation/debug.h"
#include "annotation/test.h"

// struct/exhaustion.h — the shared loud-refusal seam (the Exhaustion Loudness
// Law) for owners that own finite storage but do not embed a Collection.
//
// An owner that owns finite storage and refuses a request routes the refusal
// through Struct_reportExhaustion: the caller-supplied count always increments,
// and the epoch's FIRST refusal emits one THROW naming the owner and the
// requested size. The owner starts a fresh epoch when it is created or reset
// (Struct_resetExhaustion). Collection embeds the same accounting and delegates
// here, so every finite-storage owner reports through one implementation.

// Count a refusal and report the epoch's first one. Always returns false, so a
// caller can write `return Struct_reportExhaustion(...)`. `boundBytes` is the
// capacity the owner hit (0 when there is no meaningful bound), so the
// diagnostic names the request AND the bound (the Exhaustion Loudness Law).
;;DEBUG
bool Struct_reportExhaustion(uint64_t *count, bool *reported, const char *owner,
                             size_t requestedBytes, size_t boundBytes);

// The owner's refusal count for the current epoch.
;;TEST
uint64_t Struct_exhaustionCount(const uint64_t *count);

// Start a fresh epoch for the owner (count zeroed, diagnostic armed again).
;;TEST
void Struct_resetExhaustion(uint64_t *count, bool *reported);

// --- Construction refusals (no owner instance exists yet) ---
// A constructor that cannot allocate its struct or backing returns nullptr; the
// refusal has no per-instance counter, so it is accounted process-wide: counted
// always, reported once per process epoch. Same loudness contract, one owner.

// Count a construction refusal and report the epoch's first one. Always false.
;;DEBUG
bool Struct_reportConstructionExhaustion(const char *owner, size_t requestedBytes);

// The process-wide construction-refusal count for the current epoch.
;;TEST
uint64_t Struct_constructionExhaustionCount(void);

// Start a fresh process-wide construction epoch.
;;TEST
void Struct_resetConstructionExhaustion(void);

#endif
