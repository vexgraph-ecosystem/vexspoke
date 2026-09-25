#ifndef RELATIONAL_REFLECT_H
#define RELATIONAL_REFLECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "relational/variable_hash_map.h"

// relational/reflect.h — the Relational Engine's cold path resolver.
//
// MODULE (procedural, zero structs — like util/hash): the dotted-path walk that
// joins the reflection pieces. A SCOPE is a VariableHashMap (name => value); a
// NESTED scope is a value that is itself a VariableHashMap, whose 16-byte
// header carries the identity TYPE_VARIABLE_HASH_MAP (so Reflect_isScope reads
// the reference's own header — no side table). The path splits on '.' — the
// splitter, never a name character.
//
// COLD PATH ONLY (the Cold-Only Reflection Law): the resolver is a node walk by
// design — pointer-chasing is the right tool for "find the thing called X, right
// now", and the walk site carries the ;;INTENTION marker. It must never run on a
// frame path.

// Number of '.'-separated segments in a valid path; 0 when the path is null,
// empty, has an empty segment (leading/trailing/double dot), or a segment over
// VARIABLE_SLOT_NAME_MAX.
size_t Reflect_segmentCount(const char *path);

// Copy segment `index` (0-based) into out (bounded, NUL-terminated). Returns the
// segment length, or -1 on a null out / short buffer / index past the end.
int Reflect_segment(const char *path, size_t index, char *out, size_t cap);

// True when a resolved value is a nested scope: its header identity is
// TYPE_VARIABLE_HASH_MAP. Null-safe and foreign-safe (an out-of-arena value
// reads as 0 and returns false).
bool Reflect_isScope(uintptr_t value);

// Resolve a dotted path against the root scope (dest-last; resolved flag
// returned). On true, *outValue holds the last segment's value. Every
// intermediate segment must resolve to a nested scope. False on any bad path,
// missing name, invalid intermediate, or null inputs (a null root resolves
// nothing).
bool Reflect_resolve(VariableHashMap *root, const char *path, uintptr_t *outValue);

#endif
