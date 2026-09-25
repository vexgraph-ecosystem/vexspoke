#include "c23/equals.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "relational/symbol_table.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Equals
 * ============================================================================
 * Relational equality: the C answer to Java `.equals()`. Identity first, then
 * the block identity header (type plus length must match), then the payload
 * bytes compared directly. Foreign pointers prove identity only. The header
 * check keeps the cold path crash-free on mismatched or foreign blocks that
 * happen to collide on content.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Equals (c23/equals.c)
 * LEVEL: L2 — Behavior (relational runtime behavior)
 * ============================================================================
 * relational equality: identity, then block header, then payload bytes.
 *
 * STRUCT FIELDS: none — procedural (operates on generic Memory blocks)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - isEqual(a, b)                    : Identity + header + payload equality
 *   - isEquallyNamed(v, nameA, nameB)  : Registry form via SymbolTable symbols
 * ============================================================================
 */

bool isEqual(const void *a, const void *b) {
    if (a == b)
        return true;
    if (!a || !b)
        return false;

    uint64_t ta = Memory_type((void*) a);
    uint64_t tb = Memory_type((void*) b);
    if (ta == 0 || tb == 0)
        return false;
    if (ta != tb)
        return false;

    size_t la = Memory_length((void*) a);
    size_t lb = Memory_length((void*) b);
    if (la != lb)
        return false;

    const uint8_t *pa = (const uint8_t*) a;
    const uint8_t *pb = (const uint8_t*) b;
    return memcmp(pa, pb, la) == 0;
}

bool isEquallyNamed(SymbolTable *v, const char *nameA, const char *nameB) {
    if (!v || !nameA || !nameB)
        return false;
    int32_t idA = SymbolTable_getId(v, nameA);
    int32_t idB = SymbolTable_getId(v, nameB);
    if (idA < 0 || idB < 0)
        return false;
    if (SymbolTable_getClassId(v, idA) != SymbolTable_getClassId(v, idB))
        return false;
    const void *pa = (const void*) SymbolTable_getPointer(v, idA);
    const void *pb = (const void*) SymbolTable_getPointer(v, idB);
    return isEqual(pa, pb);
}
