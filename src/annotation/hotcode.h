#ifndef ANNOTATION_HOTCODE_H
#define ANNOTATION_HOTCODE_H

// src/annotation/hotcode.h — the hot-path declaration marker.
//
// HOTCODE: mark a function proven to run on a hot path — per frame, or inside a
// for/while loop that dominates a frame. It is the in-code declaration the
// Hot-Path Minimal Guard Law asks for: a HOTCODE function carries at most one
// entry guard (a single nullptr or range test) and returns the Contract's safe
// default, with zero per-element revalidation, zero logging, and zero
// allocation.
//
// It takes no text — the marker is the whole statement. `grep -rn ';;HOTCODE'`
// lists every hot site a change must keep cheap; a new `;;HOTCODE` without a
// matching hot-path guard test is a gap.
//
// See intention.h for the macro convention (C has no language-level
// annotations). Usage: `;;HOTCODE` on the line above the declaration.

#define HOTCODE _Static_assert(1, "@HotCode");

#endif
