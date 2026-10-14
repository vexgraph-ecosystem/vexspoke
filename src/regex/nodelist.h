#ifndef REGEX_NODELIST_H
#define REGEX_NODELIST_H

#include <stdbool.h>
#include <stdint.h>

// regex/nodelist.h — the RegexNodeList: the flat AST a parse produces and the
// program builder walks. Single Class Per File Law: RegexNodeList (owning the
// behaviorless RegexNode slot records).
//
// Data-Oriented Storage Law: the tree is a flat, index-based array. Children
// are indices, never pointers, so the array can grow (engine-arena realloc)
// without invalidating a single reference. Nothing here interprets a node;
// regex/parser.c fills it and regex/program.c emits from it.

// Not-a-node sentinel (an out-of-range or refused index).
#define REGEX_NODE_NONE UINT32_MAX

// Unbounded upper bound for a {m,} repetition.
#define REGEX_REP_INF 0xFFFFFFFFu

// What a node means. The parser writes this tag; the program builder switches
// on it.
typedef enum RegexNodeKind {
    REGEX_NODE_EMPTY = 0,  // matches the empty string
    REGEX_NODE_CHAR,       // one literal byte (already folded)
    REGEX_NODE_ANY,        // '.' — one byte, newline unless REGEX_DOTALL
    REGEX_NODE_CLASS,      // one byte in class bitmap clsIndex
    REGEX_NODE_BOL,        // '^' — zero width
    REGEX_NODE_EOL,        // '$' — zero width
    REGEX_NODE_CONCAT,     // a then b
    REGEX_NODE_ALT,        // a or b
    REGEX_NODE_STAR,       // a*
    REGEX_NODE_PLUS,       // a+
    REGEX_NODE_QUEST,      // a?
    REGEX_NODE_REPEAT,     // a{repMin,repMax}
    REGEX_NODE_GROUP       // capturing group `group` around a
} RegexNodeKind;

// SLOT RECORD: one AST node, owned by exactly one RegexNodeList, behaviorless.
typedef struct RegexNode {
    uint8_t  kind;      // RegexNodeKind
    uint8_t  byte;      // literal byte for REGEX_NODE_CHAR (already folded)
    uint16_t reserved;  // keeps the record naturally aligned
    uint32_t a;         // first child / class index
    uint32_t b;         // second child (concatenation, alternation)
    uint32_t repMin;    // REGEX_NODE_REPEAT lower bound
    uint32_t repMax;    // REGEX_NODE_REPEAT upper bound (REGEX_REP_INF = none)
    uint32_t group;     // capturing group number (0 = non-capturing)
    uint32_t clsIndex;  // class bitmap index for REGEX_NODE_CLASS
} RegexNode;

// A flat, growable array of nodes in the engine arena.
typedef struct RegexNodeList {
    RegexNode *nodes;    // engine-arena block of `capacity` nodes
    uint32_t   count;    // nodes in use
    uint32_t   capacity; // nodes allocated
} RegexNodeList;

// Zero the list; allocates nothing.
void RegexNodeList_init(RegexNodeList *list);

// Append a zeroed node of the given kind. Returns its index, or
// REGEX_NODE_NONE when the arena refused growth.
uint32_t RegexNodeList_add(RegexNodeList *list, uint8_t kind);

// Borrowed mutable node, or nullptr when index is out of range.
RegexNode *RegexNodeList_get(RegexNodeList *list, uint32_t index);

// Borrowed read-only node, or nullptr when index is out of range.
const RegexNode *RegexNodeList_at(const RegexNodeList *list, uint32_t index);

// Release the node block and zero the list.
void RegexNodeList_free(RegexNodeList *list);

#endif
