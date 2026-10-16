// regex/program.c — the RegexProgram Thompson emitter.

#include "regex/program.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: RegexProgram
 * ============================================================================
 * The compiled form of a pattern: one flat array of instructions that a
 * matcher can walk, produced here from the AST by Thompson's construction.
 * Thompson is the right construction for this engine because it never needs
 * to choose between alternatives at run time — alternation becomes a SPLIT,
 * repetition becomes a back-jump, and a capture becomes a SAVE — so the
 * resulting program has no backtracking behaviour to lose control of. That is
 * what lets the matcher stay linear on a hostile pattern (the Bounded Wait
 * Law), and it is why the AST is lowered here rather than interpreted.
 *
 * Emission is deliberately recursive over the AST: the parser caps group
 * nesting at REGEX_MAX_DEPTH, so the C stack depth is bounded by the pattern
 * that was already admitted. Repetition is expanded by copying the body —
 * {m,n} is m mandatory copies plus (n-m) optional ones — and the expansion is
 * capped by REGEX_MAX_PROGRAM, so a huge bound is a refusal, not an OOM.
 *
 * memory: one arena block stamped TYPE_REGEX, doubling on demand. The program
 * borrows the AST for the duration of build and owns nothing else.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: RegexProgram (regex/program.c — defined in regex/program.h)
 * ============================================================================
 * The flat instruction array a pattern compiles to, plus the Thompson
 * construction that emits it.
 *
 * STRUCT FIELDS (Mirroring regex/program.h):
 * ----------------------------------------------------------------------------
 *   RegexProgram {
 *     RegexInst *insts;    // engine-arena block of `capacity` instructions
 *     uint32_t   count;    // instructions in use
 *     uint32_t   capacity; // instructions allocated
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - RegexProgram() creates an empty program value
 *   - RegexProgram_build(program, nodes, root)
 *   - RegexProgram_count(program)
 *   - RegexProgram_at(program, index)
 *   - RegexProgram_free(program)
 *
 * Private Core Functions: (.c static)
 *   - programGrow(block, bytes) / emit(program, op, byte, a, b)
 *   - instAt(program, index) / emitNode(program, nodes, index)
 *
 * Admission, ownership, failure:
 *   - init zeroes the program; build emits SAVE 0, the body, SAVE 1, MATCH and
 *     returns false on ceiling breach or arena refusal (a partially emitted
 *     program must be freed, never matched).
 *   - at is a borrowed view valid until the next build or free.
 *   - free releases the block and zeroes the program; null-safe.
 * ============================================================================
 */

#define REGEX_PROGRAM_SEED_CAPACITY 64u

static void *programGrow(void *block, size_t bytes) {
    if (block)
        return Memory_realloc(block, bytes);
    return Memory_alloc(TYPE_REGEX, bytes);
}

// Appends one instruction; returns its index, or UINT32_MAX on refusal.
static uint32_t emit(RegexProgram *program, uint8_t op, uint8_t byte, uint32_t a, uint32_t b) {
    if ((*program).count >= REGEX_MAX_PROGRAM)
        return UINT32_MAX;
    if ((*program).count == (*program).capacity) {
        uint32_t next = (*program).capacity ? (*program).capacity * 2u : REGEX_PROGRAM_SEED_CAPACITY;
        if (next > REGEX_MAX_PROGRAM)
            next = REGEX_MAX_PROGRAM;
        RegexInst *grown = (RegexInst*) programGrow((*program).insts,
                                                   (size_t) next * sizeof(RegexInst));
        if (!grown)
            return UINT32_MAX;
        (*program).insts = grown;
        (*program).capacity = next;
    }
    RegexInst *inst = &(*program).insts[(*program).count];
    (*inst).op = op;
    (*inst).byte = byte;
    (*inst).reserved = 0;
    (*inst).a = a;
    (*inst).b = b;
    return (*program).count++;
}

static RegexInst *instAt(RegexProgram *program, uint32_t index) {
    return &(*program).insts[index];
}

// Emits the Thompson program for one AST node. Recursion is bounded by AST
// nesting (capped by REGEX_MAX_DEPTH); concatenation and alternation spines
// recurse once per level but their node count is capped by REGEX_MAX_NODES.
static bool emitNode(RegexProgram *program, const RegexNodeList *nodes, uint32_t index) {
    const RegexNode *node = RegexNodeList_at(nodes, index);
    if (!node)
        return false;
    switch ((*node).kind) {
    case REGEX_NODE_EMPTY:
        return true;
    case REGEX_NODE_CHAR:
        return emit(program, REGEX_OP_CHAR, (*node).byte, 0, 0) != UINT32_MAX;
    case REGEX_NODE_ANY:
        return emit(program, REGEX_OP_ANY, 0, 0, 0) != UINT32_MAX;
    case REGEX_NODE_CLASS:
        return emit(program, REGEX_OP_CLASS, 0, (*node).clsIndex, 0) != UINT32_MAX;
    case REGEX_NODE_BOL:
        return emit(program, REGEX_OP_BOL, 0, 0, 0) != UINT32_MAX;
    case REGEX_NODE_EOL:
        return emit(program, REGEX_OP_EOL, 0, 0, 0) != UINT32_MAX;
    case REGEX_NODE_CONCAT:
        return emitNode(program, nodes, (*node).a) && emitNode(program, nodes, (*node).b);
    case REGEX_NODE_ALT: {
        uint32_t split = emit(program, REGEX_OP_SPLIT, 0, 0, 0);
        if (split == UINT32_MAX)
            return false;
        (*instAt(program, split)).a = split + 1u;
        if (!emitNode(program, nodes, (*node).a))
            return false;
        uint32_t jump = emit(program, REGEX_OP_JUMP, 0, 0, 0);
        if (jump == UINT32_MAX)
            return false;
        (*instAt(program, split)).b = (*program).count;
        if (!emitNode(program, nodes, (*node).b))
            return false;
        (*instAt(program, jump)).a = (*program).count;
        return true;
    }
    case REGEX_NODE_STAR: {
        uint32_t split = emit(program, REGEX_OP_SPLIT, 0, 0, 0);
        if (split == UINT32_MAX)
            return false;
        (*instAt(program, split)).a = split + 1u;
        if (!emitNode(program, nodes, (*node).a))
            return false;
        uint32_t jump = emit(program, REGEX_OP_JUMP, 0, 0, 0);
        if (jump == UINT32_MAX)
            return false;
        (*instAt(program, jump)).a = split;
        (*instAt(program, split)).b = (*program).count;
        return true;
    }
    case REGEX_NODE_PLUS: {
        uint32_t start = (*program).count;
        if (!emitNode(program, nodes, (*node).a))
            return false;
        uint32_t split = emit(program, REGEX_OP_SPLIT, 0, 0, 0);
        if (split == UINT32_MAX)
            return false;
        (*instAt(program, split)).a = start;
        (*instAt(program, split)).b = (*program).count;
        return true;
    }
    case REGEX_NODE_QUEST: {
        uint32_t split = emit(program, REGEX_OP_SPLIT, 0, 0, 0);
        if (split == UINT32_MAX)
            return false;
        (*instAt(program, split)).a = split + 1u;
        if (!emitNode(program, nodes, (*node).a))
            return false;
        (*instAt(program, split)).b = (*program).count;
        return true;
    }
    case REGEX_NODE_REPEAT: {
        uint32_t min = (*node).repMin;
        uint32_t max = (*node).repMax;
        for (uint32_t i = 0; i < min; i++)
            if (!emitNode(program, nodes, (*node).a))
                return false;
        if (max == REGEX_REP_INF) {
            // {m,} is m copies followed by a star of the body.
            uint32_t split = emit(program, REGEX_OP_SPLIT, 0, 0, 0);
            if (split == UINT32_MAX)
                return false;
            (*instAt(program, split)).a = split + 1u;
            if (!emitNode(program, nodes, (*node).a))
                return false;
            uint32_t jump = emit(program, REGEX_OP_JUMP, 0, 0, 0);
            if (jump == UINT32_MAX)
                return false;
            (*instAt(program, jump)).a = split;
            (*instAt(program, split)).b = (*program).count;
            return true;
        }
        for (uint32_t i = min; i < max; i++) {
            uint32_t split = emit(program, REGEX_OP_SPLIT, 0, 0, 0);
            if (split == UINT32_MAX)
                return false;
            (*instAt(program, split)).a = split + 1u;
            if (!emitNode(program, nodes, (*node).a))
                return false;
            (*instAt(program, split)).b = (*program).count;
        }
        return true;
    }
    case REGEX_NODE_GROUP: {
        uint32_t group = (*node).group;
        if (group > 0)
            if (emit(program, REGEX_OP_SAVE, 0, group * 2u, 0) == UINT32_MAX)
                return false;
        if (!emitNode(program, nodes, (*node).a))
            return false;
        if (group > 0)
            if (emit(program, REGEX_OP_SAVE, 0, group * 2u + 1u, 0) == UINT32_MAX)
                return false;
        return true;
    }
    default:
        return false;
    }
}

RegexProgram RegexProgram_0(void) {
    RegexProgram program;
    program.insts = nullptr;
    program.count = 0u;
    program.capacity = 0u;
    return program;
}

bool RegexProgram_build(RegexProgram *program, const RegexNodeList *nodes, uint32_t root) {
    if (emit(program, REGEX_OP_SAVE, 0, 0, 0) == UINT32_MAX)
        return false;
    if (!emitNode(program, nodes, root))
        return false;
    if (emit(program, REGEX_OP_SAVE, 0, 1, 0) == UINT32_MAX)
        return false;
    if (emit(program, REGEX_OP_MATCH, 0, 0, 0) == UINT32_MAX)
        return false;
    return true;
}

uint32_t RegexProgram_count(const RegexProgram *program) {
    return (*program).count;
}

const RegexInst *RegexProgram_at(const RegexProgram *program, uint32_t index) {
    if (index >= (*program).count)
        return nullptr;
    return &(*program).insts[index];
}

void RegexProgram_free(RegexProgram *program) {
    if ((*program).insts)
        Memory_free((*program).insts);
    *program = RegexProgram();
}
