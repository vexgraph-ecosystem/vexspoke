#include "oop/type.h"
#include "annotation/overview.h"

#include <stddef.h>
#include <string.h>

#include "nio/mem.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Type (oop/type.c)
 * LEVEL: L1 — File Metadata (type-id metadata registry)
 * ============================================================================
 * the TypeRegister, ported from oop/TypeRegister.java.
 *
 * STRUCT FIELDS (Mirroring oop/type.h):
 * ----------------------------------------------------------------------------
 *   TypeHeader {
 *     uint64_t typeId; // block-header type id (project | form | class)
 *     uint32_t length; // payload length
 *     uint32_t pad;    // pad to 16, keeps 8-byte payloads aligned
 *   }
 *
 * PRIVATE HELPERS (kept file-local pure-data only, Rule 3):
 * ----------------------------------------------------------------------------
 *   TypeParentsRow {              // one registered project's parent chain
 *     uint64_t proj;              // owning project byte (0 = empty slot)
 *     const uint32_t *parents;    // parents[i] = parent class # of class # i
 *     uint32_t count;             // 0 = root; rows past count = root
 *   }
 *   g_typeTables                // growable slate, first-match, doubling on demand
 *   growSlate(needed)           // exponential growth, arena-backed
 *   findTable(proj)             // row lookup for the project byte
 *   vexspokeParent(cls)           // bare-id / PROJ_VEXSPOKE chain resolution
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - Type_make(proj, form, classId)
 *   - Type_class(typeId)
 *   - Type_form(typeId)
 *   - Type_registerParents(proj, parents, count)   // seam: downstream chains
 *
 * Getters:
 *   - Type_isStruct(form)
 *   - Type_isSingleton(typeId)
 *   - Type_isArray(typeId)
 *   - Type_isPointer(typeId)
 *   - Type_isStructSingleton(typeId)
 *   - Type_isStructArray(typeId)
 *   - Type_isStructSOA(typeId)
 *   - Type_isStructAOS(typeId)
 *   - Type_isStructCoexistent(typeId)
 *   - Type_isStructPointer(typeId)
 *   - Type_isPrimitive(typeId)
 *   - Type_isGlobal(typeId)
 *   - Type_isLocale(typeId)
 *   - Type_isTransient(typeId)
 *   - Type_isProactive(typeId)
 *   - Type_isReactive(typeId)
 *   - Type_isProbable(typeId)
 *   - Type_isProbableObjects(typeId)
 *   - Type_isFuture(typeId)
 *   - Type_isChoice(typeId)
 *   - Type_getParentClass(classId)
 *   - Type_isA(classId, ancestorId)
 *   - Type_arch(classId)
 * ============================================================================
 */


// type.c — TypeRegister port (Legacy: oop/TypeRegister.java).
//
// The class table lives in type.h as macros; this file carries the
// parent-chain helpers. Downstream projects ship their own *-type.h with
// per-project class numbers starting at 1 and grant central logic their
// chains once via Type_registerParents — vexspoke resolves them by project
// byte and never includes a downstream header (Rule 17).

typedef struct TypeParentsRow {
    uint64_t proj;               // owning project byte; 0 = empty slot
    const uint32_t *parents;     // parents[i] = parent class # of class # i
    uint32_t count;              // 0 row = root; rows past count = root
} TypeParentsRow;

// Growable registration slate (the Dynamic Scalability & Anti-Hardcoding
// Law): starts empty, doubles exponentially on demand, arena-backed. Rows
// beyond g_typeTableCount are never scanned, so stale bytes are harmless.
static TypeParentsRow *g_typeTables = NULL;
static size_t g_typeTableCount = 0;
static size_t g_typeTableCap = 0;

// Grow the slate to at least `needed` rows (doubling, cold start 8). On OOM
// the slate is left untouched and registration fails; the caller drops.
static bool growSlate(size_t needed) {
    if (needed <= g_typeTableCap) return true;
    size_t newCap = (g_typeTableCap == 0) ? 8 : g_typeTableCap * 2;
    while (newCap < needed) newCap *= 2;
    TypeParentsRow *nb = (TypeParentsRow*) Memory_alloc(
        TYPE_INT_POINTER, newCap * sizeof(TypeParentsRow));
    if (nb == nullptr) return false;
    if (g_typeTables != NULL && g_typeTableCap > 0)
        memcpy(nb, g_typeTables, g_typeTableCap * sizeof(TypeParentsRow));
    g_typeTables = nb;
    g_typeTableCap = newCap;
    return true;
}

static const TypeParentsRow *findTable(uint64_t proj) {
    for (size_t i = 0; i < g_typeTableCount; ++i) {
        const TypeParentsRow *row = &g_typeTables[i];
        if ((*row).proj == proj)
            return row;
    }
    return nullptr;
}

bool Type_registerParents(uint64_t proj, const uint32_t *parents, uint32_t count) {
    if (proj == 0u)
        return false;
    if ((proj & MASK_PROJECT) != proj)
        return false;
    if (proj == PROJ_VEXSPOKE)
        return false;
    if (parents == nullptr && count != 0u)
        return false;
    for (size_t i = 0; i < g_typeTableCount; ++i) {
        TypeParentsRow *row = &g_typeTables[i];
        if ((*row).proj == proj) {
            (*row).parents = parents;
            (*row).count = count;
            return true;
        }
    }
    if (!growSlate(g_typeTableCount + 1)) return false;
    TypeParentsRow *row = &g_typeTables[g_typeTableCount++];
    (*row).proj = proj;
    (*row).parents = parents;
    (*row).count = count;
    return true;
}

static uint64_t vexspokeParent(uint64_t cls) {
    if (cls >= 0x0050u && cls <= 0x0063u)          // buffer family: all->ID_BUFFER
        return ID_BUFFER;
    return cls;                                    // everything else is a root
}

uint64_t Type_getParentClass(uint64_t classId) {
    uint64_t proj = classId & MASK_PROJECT;
    if (proj == 0u || proj == PROJ_VEXSPOKE)
        return vexspokeParent(classId & MASK_CLASS);
    uint64_t cls = classId & MASK_CLASS;
    const TypeParentsRow *row = findTable(proj);
    if (row != nullptr && cls != 0u && cls < (*row).count) {
        uint32_t parent = (*row).parents[cls];
        return parent != 0u ? (uint64_t) parent : cls;   // table 0 row = root
    }
    return cls;                                    // unregistered project = root
}

uint64_t Type_arch(uint64_t classId) {
    uint64_t proj = classId & MASK_PROJECT;
    if (proj == PROJ_VEXSPOKE)
        return ARCH_VEXSPOKE;
    if (proj == PROJ_GRAPHVEX)
        return ARCH_GRAPHVEX;
    if (proj == PROJ_HOTCWAP)
        return ARCH_HOTCWAP;
    if (proj == PROJ_DARLING)
        return ARCH_DARLING;
    if (proj == PROJ_API_HAVEN)
        return ARCH_APIHAVEN;
    // Bare ids carry no project byte: with per-project numbering they can
    // only mean vexspoke's own class space (see oop/type.h). Cross-project
    // code must pass full TYPE_*_SINGLETON ids.
    return ARCH_VEXSPOKE;
}

int Type_isA(uint64_t classId, uint64_t ancestorId) {
    uint64_t proj = classId & MASK_PROJECT;
    uint64_t target = ancestorId & MASK_CLASS;
    uint64_t current = classId;
    while ((current & MASK_CLASS) != target) {
        uint64_t parent = Type_getParentClass(current);
        if ((parent & MASK_CLASS) == (current & MASK_CLASS))
            return 0;                              // root reached, not target
        current = (parent & MASK_CLASS) | proj;    // walk stays in-project
    }
    return 1;
}
