#ifndef ANNOTATION_DEBUG_H
#define ANNOTATION_DEBUG_H

// src/annotation/debug.h — the debug/diagnostic-surface marker.
//
// DEBUG: mark a function, field or entry point that exists to OBSERVE, DIAGNOSE
// or INSPECT — it carries no production semantics. A DEBUG surface may be
// compiled out (e.g. under `#if defined(DEBUG_BORROW_CHECK)`), may be slow, and
// may print. Production code may call one only on a *failure* or *inspection*
// path: the success path must never depend on it, and no correctness decision
// may rest on its result. A DEBUG surface that becomes load-bearing for
// production behaviour is a mis-annotation — promote it to a real API, never
// lean on the debug one.
//
// It takes no text — the marker is the whole statement. `grep -rn ';;DEBUG'`
// lists every debug/diagnostic surface a release build is free to drop.
//
// See intention.h for the macro convention (C has no language-level
// annotations). Usage: `;;DEBUG` on the line above the declaration.

#define DEBUG _Static_assert(1, "@Debug");

#endif
