#ifndef ANNOTATION_WHAT_H
#define ANNOTATION_WHAT_H

// src/annotation/what.h — pointee declaration marker for void*.
//
// WHAT(text): state, at the declaration site, the type a void* actually points
// at, so neither the human author nor an AI agent has to guess. The text is a
// string literal naming the C type, e.g.
//
//     ;;WHAT("uint64_t")
//     void *x;
//
//     ;;WHAT("Reactive")
//     void *health;
//
// The relational engine runs on void* — the same 8 bytes may hold a scalar, a
// struct, a reactive, or a table. This marker is the compile-time twin of the
// 16-byte self-describing memory header (the Self-Describing Memory Block Law):
// intent is legible before a single byte is allocated.
//
// See intention.h for the macro convention (C has no language-level
// annotations). Usage: `;;WHAT("...")` (see preferences.md, WHAT Law, under the
// Two-Semicolon Annotation Style Law). An annotation is not a declarator — the
// line-above form is the only compilable form; there is no inline
// `void* WHAT("...") x;`.

#define WHAT(text) _Static_assert(1, "@What(" text ")");

#endif
