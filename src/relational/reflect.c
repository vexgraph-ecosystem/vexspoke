// relational/reflect.c — the Relational Engine's cold path resolver.
//
// Split a dotted path on '.' and walk it one segment per layer: resolve the
// segment in the current scope, and (for an intermediate) descend into the
// nested scope the value names. The walk is a node walk by design; the marker
// on it is the Cold-Only Reflection Law's signpost.

#include "relational/reflect.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"
#include "oop/type.h"
#include "relational/cell.h"
#include "relational/variable_slot.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Reflect
 * ============================================================================
 * The cold path resolver: the procedural verb that answers "resolve the path
 * a.b.c.d, right now" against a scope tree. A scope is a VariableHashMap; a
 * nested scope is a value that is itself a VariableHashMap, identified by its
 * own 16-byte header (TYPE_VARIABLE_HASH_MAP) through Reflect_isScope. The path
 * splits on '.' (the splitter), each segment folds through the map's name
 * policy, and every intermediate segment must resolve to a nested scope.
 *
 * No state: a MODULE of pure functions over the existing classes, like
 * util/hash. The one marker it carries (;;INTENTION) is the deliberate
 * cold-path node walk — pointer-chasing is the tool for this rendezvous, and it
 * must never run on a frame (the Cold-Only Reflection Law).
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Reflect (relational/reflect.c)
 * LEVEL: L2 — Behavior (cold resolver verb)
 * ============================================================================
 * the cold dotted-path resolver over scope trees (zero structs — procedural).
 *
 * STRUCT FIELDS: none — procedural (operates on VariableHashMap scopes (pure
 * functions)).
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - Reflect_segmentCount(path)              : valid-segment count (0 invalid)
 *   - Reflect_segment(path, index, out, cap)  : extract one segment
 *   - Reflect_isScope(value)                  : identity check (TYPE_VARIABLE_HASH_MAP)
 *   - Reflect_resolve(root, path, outValue)   : the cold node walk
 * ============================================================================
 */

size_t Reflect_segmentCount(const char *path) {
    if (!path || path[0] == '\0')
        return 0u;
    size_t count = 0u;
    size_t len = 0u;
    for (const char *p = path; ; p++) {
        char c = *p;
        if (c != '\0' && c != '.') {
            len++;
            if (len > VARIABLE_SLOT_NAME_MAX)
                return 0u;
            continue;
        }
        if (len == 0u)
            return 0u; // empty segment: leading/trailing/double dot
        count++;
        len = 0u;
        if (c == '\0')
            return count;
    }
}

int Reflect_segment(const char *path, size_t index, char *out, size_t cap) {
    if (!path || !out || cap == 0u)
        return -1;
    if (Reflect_segmentCount(path) <= index)
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

bool Reflect_isScope(uintptr_t value) {
    if (value == 0u)
        return false;
    return Cell_check((Cell*) value, TYPE_VARIABLE_HASH_MAP);
}

bool Reflect_resolve(VariableHashMap *root, const char *path, uintptr_t *outValue) {
    if (outValue)
        *outValue = 0u;
    if (!root || !path)
        return false;

    size_t count = Reflect_segmentCount(path);
    if (count == 0u)
        return false;

    ;;INTENTION("cold path search is a node walk by design; hot iteration stays DOD")
    VariableHashMap *scope = root;
    char segment[VARIABLE_SLOT_NAME_BYTES];
    for (size_t i = 0u; i < count; i++) {
        if (Reflect_segment(path, i, segment, sizeof(segment)) < 0)
            return false;

        uintptr_t value = 0u;
        if (!VariableHashMap_get(scope, segment, &value))
            return false;

        if (i + 1u == count) {
            if (outValue)
                *outValue = value;
            return true;
        }

        // Descend: the intermediate value must itself be a scope.
        if (!Reflect_isScope(value))
            return false;
        scope = (VariableHashMap*) value;
    }
    return false;
}
