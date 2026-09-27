#ifndef ANNOTATION_CHECKER_H
#define ANNOTATION_CHECKER_H

// src/annotation/checker.h — the cold-seam validation marker.
//
// CHECKER: mark a function that validates untrusted input ONCE at a cold seam
// and returns a Try (TryValue/TryPtr) instead of a bare value — the `getTry`
// half of an accessor pair. A checker is where hostility is rejected; the hot
// read is where the already-validated handle is trusted without re-checking
// (the Cold-Strict, Hot-Minimal Validation Law).
//
// A checker is never on a hot path; it emits at most one Log_warn per failure
// and never allocates (the Failure Observability Law).
//
// It takes no text — the marker is the whole statement. `grep -rn ';;CHECKER'`
// lists every cold validator in the tree.
//
// See intention.h for the macro convention (C has no language-level
// annotations). Usage: `;;CHECKER` on the line above the declaration.

#define CHECKER _Static_assert(1, "@Checker");

#endif
