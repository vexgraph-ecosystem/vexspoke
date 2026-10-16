// regex/nodelist.c — the RegexNodeList flat AST array.

#include "regex/nodelist.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: RegexNodeList
 * ============================================================================
 * The flat AST array a parse produces. A regular expression's tree is built
 * bottom-up while scanning left to right, and its shape is only known once the
 * scan is done, so the tree cannot live in a fixed block: this list grows in
 * the engine arena and stores children as INDICES rather than pointers. That
 * is the whole reason the type exists — a reallocation moves the node block,
 * and an index into it survives the move where a pointer would not.
 *
 * It owns one arena block stamped TYPE_REGEX. Growth doubles from a named
 * default and is cold (compile time); a refused allocation returns
 * REGEX_NODE_NONE and leaves the existing nodes intact, so a failed compile
 * never leaves a half-linked tree (the Failure Atomicity and Recovery Law).
 * The list is single-threaded by construction: only the owning parser writes
 * it, and only that compile call reads it.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: RegexNodeList (regex/nodelist.c — defined in regex/nodelist.h)
 * ============================================================================
 * A growable, index-addressed AST node array in the engine arena.
 *
 * STRUCT FIELDS (Mirroring regex/nodelist.h):
 * ----------------------------------------------------------------------------
 *   RegexNodeList {
 *     RegexNode *nodes;    // engine-arena block of `capacity` nodes
 *     uint32_t   count;    // nodes in use
 *     uint32_t   capacity; // nodes allocated
 *   }
 *
 * SLOT RECORD (owned here, behaviorless):
 * ----------------------------------------------------------------------------
 *   RegexNode {
 *     uint8_t kind; uint8_t byte; uint16_t reserved;
 *     uint32_t a; uint32_t b; uint32_t repMin; uint32_t repMax;
 *     uint32_t group; uint32_t clsIndex;
 *   }
 *     a/b are child indices, group is the capture number, clsIndex names a
 *     class bitmap; the record carries no behavior of its own.
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - RegexNodeList() creates an empty list value
 *   - RegexNodeList_add(list, kind)
 *   - RegexNodeList_get(list, index)
 *   - RegexNodeList_at(list, index)
 *   - RegexNodeList_free(list)
 *
 * Private Core Functions: (.c static)
 *   - nodeGrow(block, bytes)
 *   - growToOneMore(list)
 *
 * Admission, ownership, failure:
 *   - init zeroes the list; add grows the arena block on demand and returns
 *     REGEX_NODE_NONE when the arena refuses (existing nodes preserved).
 *   - get/at are borrowed views valid until the next add or free; at accepts a
 *     const list, get does not.
 *   - free releases the block and zeroes the list; null-safe.
 * ============================================================================
 */

#define REGEX_NODELIST_SEED_CAPACITY 64u

static void *nodeGrow(void *block, size_t bytes) {
    if (block)
        return Memory_realloc(block, bytes);
    return Memory_alloc(TYPE_REGEX, bytes);
}

// Guarantees room for one more node; false when the arena refuses.
static bool growToOneMore(RegexNodeList *list) {
    if ((*list).count < (*list).capacity)
        return true;
    uint32_t next = (*list).capacity ? (*list).capacity * 2u : REGEX_NODELIST_SEED_CAPACITY;
    RegexNode *grown = (RegexNode*) nodeGrow((*list).nodes, (size_t) next * sizeof(RegexNode));
    if (!grown)
        return false;
    (*list).nodes = grown;
    (*list).capacity = next;
    return true;
}

RegexNodeList RegexNodeList_0(void) {
    RegexNodeList list;
    list.nodes = nullptr;
    list.count = 0u;
    list.capacity = 0u;
    return list;
}

uint32_t RegexNodeList_add(RegexNodeList *list, uint8_t kind) {
    if (!growToOneMore(list))
        return REGEX_NODE_NONE;
    RegexNode *node = &(*list).nodes[(*list).count];
    memset(node, 0, sizeof(RegexNode));
    (*node).kind = kind;
    return (*list).count++;
}

RegexNode *RegexNodeList_get(RegexNodeList *list, uint32_t index) {
    if (index >= (*list).count)
        return nullptr;
    return &(*list).nodes[index];
}

const RegexNode *RegexNodeList_at(const RegexNodeList *list, uint32_t index) {
    if (index >= (*list).count)
        return nullptr;
    return &(*list).nodes[index];
}

void RegexNodeList_free(RegexNodeList *list) {
    if ((*list).nodes)
        Memory_free((*list).nodes);
    *list = RegexNodeList();
}
