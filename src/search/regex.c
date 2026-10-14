// search/regex.c — the Regex class: compile-once, match-linear byte regex.

#include "search/regex.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"
#include "exception/throw.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Regex
 * ============================================================================
 * A byte-oriented regular expression compiled once and matched in linear time.
 * It exists because the ecosystem keeps needing to ask "does this text look
 * like that shape?": find/replace in editors, log and manifest scanning,
 * routing and validation in api-haven connectors, syntax tooling in language.
 * A hand-rolled strstr cannot express those shapes, and a backtracking matcher
 * hands any hostile pattern a way to hang the caller. Regex compiles the
 * pattern into a flat Thompson NFA program with 256-bit character-class
 * bitmaps (case folding resolved at compile time) and then runs a Pike-VM
 * simulation: work is bounded by subject length x program length, with no
 * backtracking and no recursion, so a pathological pattern degrades into a
 * clean non-match, never a hang (the Bounded Wait Law).
 *
 * memory: the Regex block, its source text, its instruction stream and its
 * class bitmaps all live in the engine arena and are stamped TYPE_REGEX.
 * Each match call allocates its own bounded scratch (thread lists, capture
 * rows and a work stack, all proportional to the compiled program) and frees
 * it before returning, so a compiled Regex is immutable and shareable while
 * concurrent matches never share mutable state. Node count, program size,
 * nesting depth and repeat bounds are documented safety ceilings: exceeding
 * one is a loud cold rejection (THROW), never silent truncation, because a
 * half-compiled pattern would match the wrong language.
 *
 * Concurrency: after Regex_2 returns, the Regex is read-only. Matching on the
 * same instance from several threads is safe (each call owns its scratch).
 * Compilation is a cold path; matching is a cold path — never call either on
 * a frame (the Cold-Only Reflection Law's spirit: resolve once, reuse).
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Regex (search/regex.c — defined in search/regex.h)
 * ============================================================================
 * A compiled, byte-oriented regular expression. Regex_2 parses the pattern
 * into a private AST, emits a flat Thompson-NFA instruction program plus
 * character-class bitmaps, then discards the AST. Matching is a Pike-VM
 * simulation over the program (one thorough scan, no backtracking).
 *
 * STRUCT FIELDS (Mirroring search/regex.h):
 * ----------------------------------------------------------------------------
 *   Regex {
 *     char      *pattern;        // owned copy of the source pattern text
 *     uint32_t   flags;          // REGEX_* options used at compile time
 *     uint32_t   groupCount;     // capturing groups (group 0 is the whole match)
 *     uint32_t   instCount;      // compiled instructions in use
 *     uint32_t   instCapacity;   // allocated instruction slots
 *     uint32_t   classCount;     // character-class bitmaps in use
 *     uint32_t   classCapacity;  // allocated class slots
 *     RegexInst *program;        // the compiled instruction stream
 *     uint8_t   *classes;        // classCount * REGEX_CLASS_BYTES bitmaps
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - Regex_1(pattern) / Regex_2(pattern, flags) / Regex(...) / Regex_free(regex)
 *   - Regex_pattern / Regex_flags / Regex_groupCount / Regex_instCount /
 *     Regex_classCount
 *   - Regex_isMatch / Regex_isFullMatch / Regex_find / Regex_findGroups
 *   - Regex_toString / Regex_toStringStruct
 *
 * Private Core Functions: (.c static)
 *   - lowerByte / isAsciiDigit
 *   - classSet / classTest / classAddByte / classAddRange / classNegate
 *   - classFillPredefined / isPredefinedClass
 *   - regexGrow(block, bytes)
 *   - parserFail / nodeNew / nodeAt / parserAddClass
 *   - parseEscapeByte / parseGroup / parseClass / parseAtom / parseBraces /
 *     parseRepeat / parseConcat / parseAlternation
 *   - emit / instAt / emitNode
 *   - scratchInit / scratchFree / addThread / instMatches / regexRun
 *   - textAppend / textPrintf / copyText / freeInternals
 *
 * PRIVATE HELPERS:
 * ----------------------------------------------------------------------------
 *   RegexNodeKind (enum) — AST node tags for the private compile tree.
 *   RegexNode { uint8 kind; uint8 byte; uint16 reserved; uint32 a; uint32 b;
 *               uint32 repMin; uint32 repMax; uint32 group; uint32 clsIndex; }
 *     Parser-only node: a/b are child indices, group is the capture number,
 *     clsIndex names a class bitmap, repMin/repMax carry a {m,n} bound.
 *   RegexParser { const char *pattern; size_t length; size_t pos; Regex *regex;
 *                 RegexNode *nodes; uint32 nodeCount; uint32 nodeCapacity;
 *                 uint32 groupCount; uint32 depth; bool failed;
 *                 const char *error; }
 *     Compile-time cursor over the pattern plus the growing AST node array.
 *   RegexThread { uint32 pc; uint32 capRow; }
 *     One NFA thread: a program counter and its row in the capture table.
 *   RegexList { RegexThread *threads; uint32 *seen; uint32 count;
 *               uint32 capBase; }
 *     A Thompson thread list, its per-position "already added" marks, and the
 *     base of the capture region this list writes its thread rows into.
 *   RegexTask { uint32 pc; uint32 slot; size_t value; uint8 restore; }
 *     One closure work item: expand a pc, or restore a capture slot after its
 *     subtree is done (so sibling branches never see each other's writes).
 *   RegexScratch { RegexThread *clistThreads; RegexThread *nlistThreads;
 *                  uint32 *clistSeen; uint32 *nlistSeen; RegexTask *stack;
 *                  uint32 stackCapacity; size_t *caps; size_t *work;
 *                  size_t *seed; size_t *best; uint32 capStride;
 *                  uint32 capRowCount; }
 *     Per-match-call working storage: two thread lists, the iterative closure
 *     stack, two capture regions (one row per thread slot each), the closure's
 *     working row, the all-unset seed row, and the copied winning capture row.
 *   RegexText { char *dest; size_t cap; size_t used; bool truncated; }
 *     Bounded-append cursor for the two string projections.
 *
 * Admission, ownership, failure:
 *   - Regex_1/Regex_2 admit a non-null NUL-terminated pattern. A null pattern,
 *     an unsupported escape, an unbalanced bracket, an invalid range, a
 *     malformed quantifier, or a safety-bound breach is rejected loudly
 *     (THROW) and returns nullptr; nothing half-compiled escapes.
 *   - Regex_free is null-safe and releases program, classes, pattern and block.
 *   - Regex_find / Regex_findGroups write outputs only on a match; on no match
 *     they return false and leave outputs untouched.
 *   - Matching allocates bounded scratch (the engine arena is loud on
 *     exhaustion) and always frees it, including on a no-match return.
 * ============================================================================
 */

// Documented safety ceilings. These are deliberate bounds at the owner, not
// guesses: the parser and emitter recurse per AST level, so the node ceiling
// keeps that recursion inside a normal C stack, and the program/class/repeat
// ceilings keep a hostile or accidental pattern from exhausting the arena.
;;INTENTION("Regex bounds the AST node count (8192), compiled program (32768), class count (4096), group nesting (128) and an explicit {m,n} repeat (1000). Each is a safety ceiling with a loud cold rejection, never silent truncation, so a pattern either compiles completely or not at all.")
#define REGEX_MAX_NODES   8192u
#define REGEX_MAX_PROGRAM 32768u
#define REGEX_MAX_CLASSES 4096u
#define REGEX_MAX_DEPTH   128u
#define REGEX_MAX_REPEAT  1000u

#define REGEX_NODE_NONE UINT32_MAX
#define REGEX_REP_INF   0xFFFFFFFFu

// --- private AST vocabulary ------------------------------------------------

typedef enum RegexNodeKind {
    REGEX_NODE_EMPTY = 0,
    REGEX_NODE_CHAR,
    REGEX_NODE_ANY,
    REGEX_NODE_CLASS,
    REGEX_NODE_BOL,
    REGEX_NODE_EOL,
    REGEX_NODE_CONCAT,
    REGEX_NODE_ALT,
    REGEX_NODE_STAR,
    REGEX_NODE_PLUS,
    REGEX_NODE_QUEST,
    REGEX_NODE_REPEAT,
    REGEX_NODE_GROUP
} RegexNodeKind;

typedef struct RegexNode {
    uint8_t  kind;
    uint8_t  byte;
    uint16_t reserved;
    uint32_t a;
    uint32_t b;
    uint32_t repMin;
    uint32_t repMax;
    uint32_t group;
    uint32_t clsIndex;
} RegexNode;

typedef struct RegexParser {
    const char *pattern;
    size_t      length;
    size_t      pos;
    Regex      *regex;
    RegexNode  *nodes;
    uint32_t    nodeCount;
    uint32_t    nodeCapacity;
    uint32_t    groupCount;
    uint32_t    depth;
    bool        failed;
    const char *error;
} RegexParser;

typedef struct RegexThread {
    uint32_t pc;
    uint32_t capRow;
} RegexThread;

typedef struct RegexList {
    RegexThread *threads;
    uint32_t    *seen;
    uint32_t     count;
    uint32_t     capBase;  // first capture row owned by this list
} RegexList;

// One closure work item: either "expand this pc" or "restore a capture slot".
typedef struct RegexTask {
    uint32_t pc;
    uint32_t slot;
    size_t   value;
    uint8_t  restore;
} RegexTask;

typedef struct RegexScratch {
    RegexThread *clistThreads;
    RegexThread *nlistThreads;
    uint32_t    *clistSeen;
    uint32_t    *nlistSeen;
    RegexTask   *stack;
    uint32_t     stackCapacity;
    size_t      *caps;          // two regions of capRowCount rows each
    size_t      *work;          // the closure's working capture row
    size_t      *seed;          // the all-unset row every search starts from
    size_t      *best;          // the winning capture row, copied on match
    uint32_t     capStride;
    uint32_t     capRowCount;
} RegexScratch;

typedef struct RegexText {
    char  *dest;
    size_t cap;
    size_t used;
    bool   truncated;
} RegexText;

// ===========================================================================
// Small byte/bit helpers (ASCII folding; the engine is byte-oriented, so no
// locale-dependent ctype is used).
// ===========================================================================

// Lowercases an ASCII letter; every other byte is returned unchanged.
static uint8_t lowerByte(uint8_t c) {
    if (c >= (uint8_t) 'A' && c <= (uint8_t) 'Z')
        return (uint8_t)(c + 32u);
    return c;
}

static bool isAsciiDigit(char c) {
    return c >= '0' && c <= '9';
}

// Sets one byte's bit in a 256-bit class bitmap.
static void classSet(uint8_t *bitmap, uint8_t byte) {
    bitmap[byte >> 3] |= (uint8_t)(1u << (byte & 7u));
}

// Tests one byte's bit in a 256-bit class bitmap.
static bool classTest(const uint8_t *bitmap, uint8_t byte) {
    return (bitmap[byte >> 3] & (uint8_t)(1u << (byte & 7u))) != 0;
}

// Adds a byte, and its opposite case when folding is on (letters only).
static void classAddByte(uint8_t *bitmap, uint8_t byte, bool insensitive) {
    classSet(bitmap, byte);
    if (!insensitive)
        return;
    if (byte >= (uint8_t) 'A' && byte <= (uint8_t) 'Z')
        classSet(bitmap, (uint8_t)(byte + 32u));
    else if (byte >= (uint8_t) 'a' && byte <= (uint8_t) 'z')
        classSet(bitmap, (uint8_t)(byte - 32u));
}

// Adds an inclusive byte range [lo, hi], folding both cases when asked.
static void classAddRange(uint8_t *bitmap, uint8_t lo, uint8_t hi, bool insensitive) {
    for (uint32_t c = lo; c <= (uint32_t) hi; c++)
        classSet(bitmap, (uint8_t) c);
    if (insensitive) {
        for (uint32_t c = lo; c <= (uint32_t) hi; c++) {
            uint8_t up = (uint8_t) c;
            if (up >= (uint8_t) 'A' && up <= (uint8_t) 'Z')
                classSet(bitmap, (uint8_t)(up + 32u));
            else if (up >= (uint8_t) 'a' && up <= (uint8_t) 'z')
                classSet(bitmap, (uint8_t)(up - 32u));
        }
    }
}

// Complements a class bitmap in place.
static void classNegate(uint8_t *bitmap) {
    for (uint32_t i = 0; i < REGEX_CLASS_BYTES; i++)
        bitmap[i] = (uint8_t) ~bitmap[i];
}

// Grows an arena block, stamping TYPE_REGEX on the first allocation.
static void *regexGrow(void *block, size_t bytes) {
    if (block)
        return Memory_realloc(block, bytes);
    return Memory_alloc(TYPE_REGEX, bytes);
}

// ===========================================================================
// AST construction (the private parser)
// ===========================================================================

static void parserFail(RegexParser *p, const char *message) {
    if (!(*p).failed) {
        (*p).failed = true;
        (*p).error = message;
    }
}

static RegexNode *nodeAt(RegexParser *p, uint32_t index) {
    return &(*p).nodes[index];
}

// Appends a fresh zeroed node and returns its index, or REGEX_NODE_NONE.
static uint32_t nodeNew(RegexParser *p, uint8_t kind) {
    if ((*p).nodeCount >= REGEX_MAX_NODES) {
        parserFail(p, "pattern too complex (node ceiling)");
        return REGEX_NODE_NONE;
    }
    if ((*p).nodeCount == (*p).nodeCapacity) {
        uint32_t next = (*p).nodeCapacity ? (*p).nodeCapacity * 2u : 64u;
        if (next > REGEX_MAX_NODES)
            next = REGEX_MAX_NODES;
        RegexNode *grown = (RegexNode*) regexGrow((*p).nodes, (size_t) next * sizeof(RegexNode));
        if (!grown) {
            parserFail(p, "out of memory");
            return REGEX_NODE_NONE;
        }
        (*p).nodes = grown;
        (*p).nodeCapacity = next;
    }
    RegexNode *node = nodeAt(p, (*p).nodeCount);
    memset(node, 0, sizeof(RegexNode));
    (*node).kind = kind;
    return (*p).nodeCount++;
}

static uint32_t parserAddClass(RegexParser *p) {
    Regex *re = (*p).regex;
    if ((*re).classCount >= REGEX_MAX_CLASSES) {
        parserFail(p, "too many character classes");
        return UINT32_MAX;
    }
    if ((*re).classCount == (*re).classCapacity) {
        uint32_t next = (*re).classCapacity ? (*re).classCapacity * 2u : 8u;
        if (next > REGEX_MAX_CLASSES)
            next = REGEX_MAX_CLASSES;
        uint8_t *grown = (uint8_t*) regexGrow((*re).classes, (size_t) next * REGEX_CLASS_BYTES);
        if (!grown) {
            parserFail(p, "out of memory");
            return UINT32_MAX;
        }
        (*re).classes = grown;
        (*re).classCapacity = next;
    }
    uint32_t index = (*re).classCount++;
    memset((*re).classes + (size_t) index * REGEX_CLASS_BYTES, 0, REGEX_CLASS_BYTES);
    return index;
}

// Fills a bitmap with the \d/\w/\s family (and their complements when upper).
static void classFillPredefined(uint8_t *bitmap, char letter) {
    switch (letter) {
    case 'd':
        for (uint32_t c = '0'; c <= '9'; c++)
            classSet(bitmap, (uint8_t) c);
        break;
    case 'D':
        for (uint32_t c = 0; c < 256u; c++)
            if (c < '0' || c > '9')
                classSet(bitmap, (uint8_t) c);
        break;
    case 'w':
        for (uint32_t c = '0'; c <= '9'; c++) classSet(bitmap, (uint8_t) c);
        for (uint32_t c = 'A'; c <= 'Z'; c++) classSet(bitmap, (uint8_t) c);
        for (uint32_t c = 'a'; c <= 'z'; c++) classSet(bitmap, (uint8_t) c);
        classSet(bitmap, (uint8_t) '_');
        break;
    case 'W':
        for (uint32_t c = 0; c < 256u; c++) {
            bool word = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') || c == '_';
            if (!word)
                classSet(bitmap, (uint8_t) c);
        }
        break;
    case 's':
        classSet(bitmap, (uint8_t) ' ');
        classSet(bitmap, (uint8_t) '\t');
        classSet(bitmap, (uint8_t) '\n');
        classSet(bitmap, (uint8_t) '\r');
        classSet(bitmap, (uint8_t) '\f');
        classSet(bitmap, (uint8_t) '\v');
        break;
    case 'S':
        for (uint32_t c = 0; c < 256u; c++) {
            bool space = c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
                         c == '\f' || c == '\v';
            if (!space)
                classSet(bitmap, (uint8_t) c);
        }
        break;
    default:
        break;
    }
}

static bool isPredefinedClass(char letter) {
    return letter == 'd' || letter == 'D' || letter == 'w' || letter == 'W' ||
           letter == 's' || letter == 'S';
}

// Reads the escape body after a '\' and yields a literal byte. Returns false
// when the escape is unsupported (a predefined class is handled by the caller
// before reaching here).
static bool parseEscapeByte(RegexParser *p, uint8_t *outByte) {
    if ((*p).pos >= (*p).length) {
        parserFail(p, "trailing backslash");
        return false;
    }
    char c = (*p).pattern[(*p).pos++];
    switch (c) {
    case 'n': *outByte = (uint8_t) '\n'; return true;
    case 'r': *outByte = (uint8_t) '\r'; return true;
    case 't': *outByte = (uint8_t) '\t'; return true;
    case 'f': *outByte = (uint8_t) '\f'; return true;
    case 'v': *outByte = (uint8_t) '\v'; return true;
    case '0': *outByte = (uint8_t) '\0'; return true;
    case 'x': {
        if ((*p).pos + 1u >= (*p).length) {
            parserFail(p, "malformed \\x escape");
            return false;
        }
        uint32_t hi = 0, lo = 0;
        char h = (*p).pattern[(*p).pos];
        char l = (*p).pattern[(*p).pos + 1u];
        if (h >= '0' && h <= '9') hi = (uint32_t)(h - '0');
        else if (h >= 'a' && h <= 'f') hi = (uint32_t)(h - 'a' + 10);
        else if (h >= 'A' && h <= 'F') hi = (uint32_t)(h - 'A' + 10);
        else { parserFail(p, "malformed \\x escape"); return false; }
        if (l >= '0' && l <= '9') lo = (uint32_t)(l - '0');
        else if (l >= 'a' && l <= 'f') lo = (uint32_t)(l - 'a' + 10);
        else if (l >= 'A' && l <= 'F') lo = (uint32_t)(l - 'A' + 10);
        else { parserFail(p, "malformed \\x escape"); return false; }
        (*p).pos += 2u;
        *outByte = (uint8_t)((hi << 4) | lo);
        return true;
    }
    default:
        // Any other escaped character is a literal (\. \* \\ \/ \- ...).
        *outByte = (uint8_t) c;
        return true;
    }
}

static uint32_t parseAlternation(RegexParser *p);

// Parses "(?:...)" or "(...)" into a group node (capturing groups get SAVE
// slots at emit time).
static uint32_t parseGroup(RegexParser *p) {
    (*p).pos++; // consume '('
    bool capturing = true;
    if ((*p).pos + 1u < (*p).length && (*p).pattern[(*p).pos] == '?' &&
        (*p).pattern[(*p).pos + 1u] == ':') {
        capturing = false;
        (*p).pos += 2u;
    } else if ((*p).pos < (*p).length && (*p).pattern[(*p).pos] == '?') {
        parserFail(p, "unsupported group construct (only (?:...) is supported)");
        return REGEX_NODE_NONE;
    }
    uint32_t group = 0;
    if (capturing)
        group = ++(*p).groupCount;
    if ((*p).depth >= REGEX_MAX_DEPTH) {
        parserFail(p, "pattern nesting too deep");
        return REGEX_NODE_NONE;
    }
    (*p).depth++;
    uint32_t body = parseAlternation(p);
    (*p).depth--;
    if (body == REGEX_NODE_NONE)
        return REGEX_NODE_NONE;
    if ((*p).pos >= (*p).length || (*p).pattern[(*p).pos] != ')') {
        parserFail(p, "missing closing ')'");
        return REGEX_NODE_NONE;
    }
    (*p).pos++;
    if (!capturing)
        return body;
    uint32_t node = nodeNew(p, REGEX_NODE_GROUP);
    if (node == REGEX_NODE_NONE)
        return REGEX_NODE_NONE;
    RegexNode *n = nodeAt(p, node);
    (*n).a = body;
    (*n).group = group;
    return node;
}

// Parses "[...]" into a class bitmap index, or UINT32_MAX on failure.
static uint32_t parseClass(RegexParser *p) {
    Regex *re = (*p).regex;
    (*p).pos++; // consume '['
    bool negate = false;
    if ((*p).pos < (*p).length && (*p).pattern[(*p).pos] == '^') {
        negate = true;
        (*p).pos++;
    }
    uint32_t index = parserAddClass(p);
    if (index == UINT32_MAX)
        return UINT32_MAX;
    uint8_t *bitmap = (*re).classes + (size_t) index * REGEX_CLASS_BYTES;
    bool insensitive = ((*re).flags & REGEX_CASE_INSENSITIVE) != 0;
    bool first = true;
    bool closed = false;
    while ((*p).pos < (*p).length) {
        char c = (*p).pattern[(*p).pos];
        if (c == ']' && !first) {
            (*p).pos++;
            closed = true;
            break;
        }
        first = false;
        uint8_t lo = 0;
        bool predefined = false;
        if (c == '\\') {
            (*p).pos++;
            if ((*p).pos < (*p).length && isPredefinedClass((*p).pattern[(*p).pos])) {
                classFillPredefined(bitmap, (*p).pattern[(*p).pos]);
                (*p).pos++;
                predefined = true;
            } else if (!parseEscapeByte(p, &lo)) {
                return UINT32_MAX;
            }
        } else {
            lo = (uint8_t) c;
            (*p).pos++;
        }
        if (predefined)
            continue;
        if ((*p).pos + 1u < (*p).length && (*p).pattern[(*p).pos] == '-' &&
            (*p).pattern[(*p).pos + 1u] != ']') {
            (*p).pos++; // consume '-'
            uint8_t hi = 0;
            char hc = (*p).pattern[(*p).pos];
            if (hc == '\\') {
                (*p).pos++;
                if ((*p).pos < (*p).length && isPredefinedClass((*p).pattern[(*p).pos])) {
                    parserFail(p, "character class in a range endpoint");
                    return UINT32_MAX;
                }
                if (!parseEscapeByte(p, &hi))
                    return UINT32_MAX;
            } else {
                hi = (uint8_t) hc;
                (*p).pos++;
            }
            if (hi < lo) {
                parserFail(p, "invalid character range (end before start)");
                return UINT32_MAX;
            }
            classAddRange(bitmap, lo, hi, insensitive);
        } else {
            classAddByte(bitmap, lo, insensitive);
        }
    }
    if (!closed) {
        parserFail(p, "missing closing ']'");
        return UINT32_MAX;
    }
    if (negate)
        classNegate(bitmap);
    return index;
}

// Parses one atom: a literal, '.', anchor, group, class or escape.
static uint32_t parseAtom(RegexParser *p) {
    char c = (*p).pattern[(*p).pos];
    Regex *re = (*p).regex;
    switch (c) {
    case '(':
        return parseGroup(p);
    case '[': {
        uint32_t index = parseClass(p);
        if (index == UINT32_MAX)
            return REGEX_NODE_NONE;
        uint32_t node = nodeNew(p, REGEX_NODE_CLASS);
        if (node == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        (*nodeAt(p, node)).clsIndex = index;
        return node;
    }
    case '.': {
        (*p).pos++;
        return nodeNew(p, REGEX_NODE_ANY);
    }
    case '^': {
        (*p).pos++;
        return nodeNew(p, REGEX_NODE_BOL);
    }
    case '$': {
        (*p).pos++;
        return nodeNew(p, REGEX_NODE_EOL);
    }
    case '\\': {
        (*p).pos++;
        if ((*p).pos < (*p).length && isPredefinedClass((*p).pattern[(*p).pos])) {
            uint32_t index = parserAddClass(p);
            if (index == UINT32_MAX)
                return REGEX_NODE_NONE;
            classFillPredefined((*re).classes + (size_t) index * REGEX_CLASS_BYTES,
                                (*p).pattern[(*p).pos]);
            (*p).pos++;
            uint32_t node = nodeNew(p, REGEX_NODE_CLASS);
            if (node == REGEX_NODE_NONE)
                return REGEX_NODE_NONE;
            (*nodeAt(p, node)).clsIndex = index;
            return node;
        }
        uint8_t byte = 0;
        if (!parseEscapeByte(p, &byte))
            return REGEX_NODE_NONE;
        uint32_t node = nodeNew(p, REGEX_NODE_CHAR);
        if (node == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        (*nodeAt(p, node)).byte = ((*re).flags & REGEX_CASE_INSENSITIVE) ? lowerByte(byte) : byte;
        return node;
    }
    case '*':
    case '+':
    case '?':
        parserFail(p, "nothing to repeat");
        return REGEX_NODE_NONE;
    default: {
        (*p).pos++;
        uint32_t node = nodeNew(p, REGEX_NODE_CHAR);
        if (node == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        uint8_t byte = (uint8_t) c;
        (*nodeAt(p, node)).byte = ((*re).flags & REGEX_CASE_INSENSITIVE) ? lowerByte(byte) : byte;
        return node;
    }
    }
}

// Parses "{m}", "{m,}" or "{m,n}". Returns 1 when a quantifier was consumed,
// 0 when '{' is an ordinary literal, and -1 when the quantifier is malformed.
static int parseBraces(RegexParser *p, uint32_t *outMin, uint32_t *outMax, bool *outHasMax) {
    size_t i = (*p).pos + 1u;
    if (i >= (*p).length || !isAsciiDigit((*p).pattern[i]))
        return 0;
    uint32_t lo = 0;
    while (i < (*p).length && isAsciiDigit((*p).pattern[i])) {
        lo = lo * 10u + (uint32_t)((*p).pattern[i] - '0');
        if (lo > REGEX_MAX_REPEAT) {
            parserFail(p, "repetition count too large");
            return -1;
        }
        i++;
    }
    bool hasMax = true;
    uint32_t hi = lo;
    if (i < (*p).length && (*p).pattern[i] == ',') {
        i++;
        if (i < (*p).length && isAsciiDigit((*p).pattern[i])) {
            hi = 0;
            while (i < (*p).length && isAsciiDigit((*p).pattern[i])) {
                hi = hi * 10u + (uint32_t)((*p).pattern[i] - '0');
                if (hi > REGEX_MAX_REPEAT) {
                    parserFail(p, "repetition count too large");
                    return -1;
                }
                i++;
            }
        } else {
            hasMax = false;
        }
    }
    if (i >= (*p).length || (*p).pattern[i] != '}') {
        parserFail(p, "malformed repetition");
        return -1;
    }
    if (hasMax && hi < lo) {
        parserFail(p, "invalid repetition range (end before start)");
        return -1;
    }
    (*p).pos = i + 1u;
    *outMin = lo;
    *outMax = hasMax ? hi : REGEX_REP_INF;
    *outHasMax = hasMax;
    return 1;
}

// Parses an atom plus any trailing quantifiers into one node.
static uint32_t parseRepeat(RegexParser *p) {
    uint32_t atom = parseAtom(p);
    if (atom == REGEX_NODE_NONE)
        return REGEX_NODE_NONE;
    for (;;) {
        if ((*p).pos >= (*p).length)
            break;
        char c = (*p).pattern[(*p).pos];
        uint8_t wrapKind = 0;
        uint32_t min = 0, max = 0;
        bool hasMax = false;
        if (c == '*') {
            wrapKind = REGEX_NODE_STAR;
            (*p).pos++;
        } else if (c == '+') {
            wrapKind = REGEX_NODE_PLUS;
            (*p).pos++;
        } else if (c == '?') {
            wrapKind = REGEX_NODE_QUEST;
            (*p).pos++;
        } else if (c == '{') {
            int braces = parseBraces(p, &min, &max, &hasMax);
            if (braces < 0)
                return REGEX_NODE_NONE;
            if (braces == 0)
                break; // '{' is a literal; leave it for the concat loop
            wrapKind = REGEX_NODE_REPEAT;
        } else {
            break;
        }
        uint32_t wrap = nodeNew(p, wrapKind);
        if (wrap == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        RegexNode *n = nodeAt(p, wrap);
        (*n).a = atom;
        if (wrapKind == REGEX_NODE_REPEAT) {
            (*n).repMin = min;
            (*n).repMax = hasMax ? max : REGEX_REP_INF;
        }
        atom = wrap;
    }
    return atom;
}

// Parses a run of repeat atoms into a left-nested concatenation.
static uint32_t parseConcat(RegexParser *p) {
    uint32_t head = REGEX_NODE_NONE;
    for (;;) {
        if ((*p).pos >= (*p).length)
            break;
        char c = (*p).pattern[(*p).pos];
        if (c == '|' || c == ')')
            break;
        uint32_t piece = parseRepeat(p);
        if (piece == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        if (head == REGEX_NODE_NONE) {
            head = piece;
            continue;
        }
        uint32_t cat = nodeNew(p, REGEX_NODE_CONCAT);
        if (cat == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        RegexNode *n = nodeAt(p, cat);
        (*n).a = head;
        (*n).b = piece;
        head = cat;
    }
    if (head == REGEX_NODE_NONE)
        return nodeNew(p, REGEX_NODE_EMPTY);
    return head;
}

// Parses a run of concatenations separated by '|' into a left-nested alternation.
static uint32_t parseAlternation(RegexParser *p) {
    uint32_t left = parseConcat(p);
    if (left == REGEX_NODE_NONE)
        return REGEX_NODE_NONE;
    while ((*p).pos < (*p).length && (*p).pattern[(*p).pos] == '|') {
        (*p).pos++;
        uint32_t right = parseConcat(p);
        if (right == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        uint32_t alt = nodeNew(p, REGEX_NODE_ALT);
        if (alt == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        RegexNode *n = nodeAt(p, alt);
        (*n).a = left;
        (*n).b = right;
        left = alt;
    }
    return left;
}

// ===========================================================================
// Program emission (Thompson construction over the AST)
// ===========================================================================

// Appends one instruction; returns its index, or UINT32_MAX on overflow.
static uint32_t emit(Regex *re, uint8_t op, uint8_t byte, uint32_t a, uint32_t b) {
    if ((*re).instCount >= REGEX_MAX_PROGRAM)
        return UINT32_MAX;
    if ((*re).instCount == (*re).instCapacity) {
        uint32_t next = (*re).instCapacity ? (*re).instCapacity * 2u : 64u;
        if (next > REGEX_MAX_PROGRAM)
            next = REGEX_MAX_PROGRAM;
        RegexInst *grown = (RegexInst*) regexGrow((*re).program, (size_t) next * sizeof(RegexInst));
        if (!grown)
            return UINT32_MAX;
        (*re).program = grown;
        (*re).instCapacity = next;
    }
    RegexInst *inst = &(*re).program[(*re).instCount];
    (*inst).op = op;
    (*inst).byte = byte;
    (*inst).reserved = 0;
    (*inst).a = a;
    (*inst).b = b;
    return (*re).instCount++;
}

static RegexInst *instAt(Regex *re, uint32_t index) {
    return &(*re).program[index];
}

// Emits the Thompson program for one AST node. Recursion depth is bounded by
// the AST nesting (groups and quantifier wrappers), which the parser caps at
// REGEX_MAX_DEPTH; concatenation and alternation spines recurse once per level
// but their node count is capped at REGEX_MAX_NODES.
static bool emitNode(Regex *re, const RegexNode *nodes, uint32_t index) {
    const RegexNode *n = &nodes[index];
    switch ((*n).kind) {
    case REGEX_NODE_EMPTY:
        return true;
    case REGEX_NODE_CHAR:
        return emit(re, REGEX_OP_CHAR, (*n).byte, 0, 0) != UINT32_MAX;
    case REGEX_NODE_ANY:
        return emit(re, REGEX_OP_ANY, 0, 0, 0) != UINT32_MAX;
    case REGEX_NODE_CLASS:
        return emit(re, REGEX_OP_CLASS, 0, (*n).clsIndex, 0) != UINT32_MAX;
    case REGEX_NODE_BOL:
        return emit(re, REGEX_OP_BOL, 0, 0, 0) != UINT32_MAX;
    case REGEX_NODE_EOL:
        return emit(re, REGEX_OP_EOL, 0, 0, 0) != UINT32_MAX;
    case REGEX_NODE_CONCAT:
        return emitNode(re, nodes, (*n).a) && emitNode(re, nodes, (*n).b);
    case REGEX_NODE_ALT: {
        uint32_t split = emit(re, REGEX_OP_SPLIT, 0, 0, 0);
        if (split == UINT32_MAX)
            return false;
        (*instAt(re, split)).a = split + 1u;
        if (!emitNode(re, nodes, (*n).a))
            return false;
        uint32_t jump = emit(re, REGEX_OP_JUMP, 0, 0, 0);
        if (jump == UINT32_MAX)
            return false;
        (*instAt(re, split)).b = (*re).instCount;
        if (!emitNode(re, nodes, (*n).b))
            return false;
        (*instAt(re, jump)).a = (*re).instCount;
        return true;
    }
    case REGEX_NODE_STAR: {
        uint32_t split = emit(re, REGEX_OP_SPLIT, 0, 0, 0);
        if (split == UINT32_MAX)
            return false;
        (*instAt(re, split)).a = split + 1u;
        if (!emitNode(re, nodes, (*n).a))
            return false;
        uint32_t jump = emit(re, REGEX_OP_JUMP, 0, 0, 0);
        if (jump == UINT32_MAX)
            return false;
        (*instAt(re, jump)).a = split;
        (*instAt(re, split)).b = (*re).instCount;
        return true;
    }
    case REGEX_NODE_PLUS: {
        uint32_t start = (*re).instCount;
        if (!emitNode(re, nodes, (*n).a))
            return false;
        uint32_t split = emit(re, REGEX_OP_SPLIT, 0, 0, 0);
        if (split == UINT32_MAX)
            return false;
        (*instAt(re, split)).a = start;
        (*instAt(re, split)).b = (*re).instCount;
        return true;
    }
    case REGEX_NODE_QUEST: {
        uint32_t split = emit(re, REGEX_OP_SPLIT, 0, 0, 0);
        if (split == UINT32_MAX)
            return false;
        (*instAt(re, split)).a = split + 1u;
        if (!emitNode(re, nodes, (*n).a))
            return false;
        (*instAt(re, split)).b = (*re).instCount;
        return true;
    }
    case REGEX_NODE_REPEAT: {
        uint32_t min = (*n).repMin;
        uint32_t max = (*n).repMax;
        for (uint32_t i = 0; i < min; i++)
            if (!emitNode(re, nodes, (*n).a))
                return false;
        if (max == REGEX_REP_INF) {
            // {m,} == m copies followed by a star of the body.
            uint32_t split = emit(re, REGEX_OP_SPLIT, 0, 0, 0);
            if (split == UINT32_MAX)
                return false;
            (*instAt(re, split)).a = split + 1u;
            if (!emitNode(re, nodes, (*n).a))
                return false;
            uint32_t jump = emit(re, REGEX_OP_JUMP, 0, 0, 0);
            if (jump == UINT32_MAX)
                return false;
            (*instAt(re, jump)).a = split;
            (*instAt(re, split)).b = (*re).instCount;
            return true;
        }
        for (uint32_t i = min; i < max; i++) {
            uint32_t split = emit(re, REGEX_OP_SPLIT, 0, 0, 0);
            if (split == UINT32_MAX)
                return false;
            (*instAt(re, split)).a = split + 1u;
            if (!emitNode(re, nodes, (*n).a))
                return false;
            (*instAt(re, split)).b = (*re).instCount;
        }
        return true;
    }
    case REGEX_NODE_GROUP: {
        uint32_t group = (*n).group;
        if (group > 0)
            if (emit(re, REGEX_OP_SAVE, 0, group * 2u, 0) == UINT32_MAX)
                return false;
        if (!emitNode(re, nodes, (*n).a))
            return false;
        if (group > 0)
            if (emit(re, REGEX_OP_SAVE, 0, group * 2u + 1u, 0) == UINT32_MAX)
                return false;
        return true;
    }
    default:
        return false;
    }
}

// ===========================================================================
// Matching (Pike VM): bounded, backtracking-free, iterative
// ===========================================================================

static void scratchFree(RegexScratch *sc) {
    if ((*sc).clistThreads) Memory_free((*sc).clistThreads);
    if ((*sc).nlistThreads) Memory_free((*sc).nlistThreads);
    if ((*sc).clistSeen) Memory_free((*sc).clistSeen);
    if ((*sc).nlistSeen) Memory_free((*sc).nlistSeen);
    if ((*sc).stack) Memory_free((*sc).stack);
    if ((*sc).caps) Memory_free((*sc).caps);
    if ((*sc).work) Memory_free((*sc).work);
    if ((*sc).seed) Memory_free((*sc).seed);
    if ((*sc).best) Memory_free((*sc).best);
    memset(sc, 0, sizeof(RegexScratch));
}

static bool scratchInit(RegexScratch *sc, const Regex *re) {
    uint32_t n = (*re).instCount;
    if (n == 0)
        return false;
    (*sc).capStride = 2u * ((*re).groupCount + 1u);
    (*sc).capRowCount = n; // one capture row per thread slot, per region
    (*sc).stackCapacity = 4u * n + 8u;
    (*sc).clistThreads = (RegexThread*) Memory_alloc(TYPE_REGEX, (size_t) n * sizeof(RegexThread));
    (*sc).nlistThreads = (RegexThread*) Memory_alloc(TYPE_REGEX, (size_t) n * sizeof(RegexThread));
    (*sc).clistSeen = (uint32_t*) Memory_alloc(TYPE_REGEX, (size_t) n * sizeof(uint32_t));
    (*sc).nlistSeen = (uint32_t*) Memory_alloc(TYPE_REGEX, (size_t) n * sizeof(uint32_t));
    (*sc).stack = (RegexTask*) Memory_alloc(TYPE_REGEX, (size_t)(*sc).stackCapacity * sizeof(RegexTask));
    (*sc).caps = (size_t*) Memory_alloc(TYPE_REGEX, 2u * (size_t) n * (*sc).capStride * sizeof(size_t));
    (*sc).work = (size_t*) Memory_alloc(TYPE_REGEX, (size_t)(*sc).capStride * sizeof(size_t));
    (*sc).seed = (size_t*) Memory_alloc(TYPE_REGEX, (size_t)(*sc).capStride * sizeof(size_t));
    (*sc).best = (size_t*) Memory_alloc(TYPE_REGEX, (size_t)(*sc).capStride * sizeof(size_t));
    if (!(*sc).clistThreads || !(*sc).nlistThreads || !(*sc).clistSeen || !(*sc).nlistSeen ||
        !(*sc).stack || !(*sc).caps || !(*sc).work || !(*sc).seed || !(*sc).best) {
        scratchFree(sc);
        return false;
    }
    // Every capture slot starts as REGEX_UNSET (all bits set).
    memset((*sc).caps, 0xFF, 2u * (size_t) n * (*sc).capStride * sizeof(size_t));
    memset((*sc).seed, 0xFF, (size_t)(*sc).capStride * sizeof(size_t));
    memset((*sc).best, 0xFF, (size_t)(*sc).capStride * sizeof(size_t));
    return true;
}

// Epsilon-closure: adds pc0 and everything reachable without consuming a byte,
// with parentCaps as the starting capture row. The walk is iterative (an
// explicit work stack), so a deep program cannot recurse the C stack, and a
// capture write is paired with a restore marker so sibling branches never see
// each other's slots.
static void addThread(const Regex *re, RegexScratch *sc, RegexList *list, uint32_t pc0,
                      const size_t *parentCaps, size_t pos, const char *subject,
                      size_t length, bool multiline) {
    uint32_t stride = (*sc).capStride;
    size_t *work = (*sc).work;
    RegexTask *stack = (*sc).stack;
    uint32_t sp = 0;
    memcpy(work, parentCaps, (size_t) stride * sizeof(size_t));
    stack[sp].pc = pc0;
    stack[sp].restore = 0;
    sp++;
    while (sp > 0) {
        sp--;
        RegexTask task = stack[sp];
        if (task.restore) {
            work[task.slot] = task.value;
            continue;
        }
        if ((*list).seen[task.pc])
            continue;
        (*list).seen[task.pc] = 1u;
        RegexInst *inst = &(*re).program[task.pc];
        switch ((*inst).op) {
        case REGEX_OP_JUMP:
            stack[sp].pc = (*inst).a;
            stack[sp].restore = 0;
            sp++;
            break;
        case REGEX_OP_SPLIT:
            stack[sp].pc = (*inst).b;
            stack[sp].restore = 0;
            sp++;
            stack[sp].pc = (*inst).a;
            stack[sp].restore = 0;
            sp++;
            break;
        case REGEX_OP_SAVE: {
            uint32_t slot = (*inst).a;
            stack[sp].pc = 0;
            stack[sp].slot = slot;
            stack[sp].value = work[slot];
            stack[sp].restore = 1;
            sp++;
            stack[sp].pc = task.pc + 1u;
            stack[sp].restore = 0;
            sp++;
            work[slot] = pos;
            break;
        }
        case REGEX_OP_BOL:
            if (pos == 0 || (multiline && subject[pos - 1] == '\n')) {
                stack[sp].pc = task.pc + 1u;
                stack[sp].restore = 0;
                sp++;
            }
            break;
        case REGEX_OP_EOL:
            if (pos == length || (multiline && subject[pos] == '\n')) {
                stack[sp].pc = task.pc + 1u;
                stack[sp].restore = 0;
                sp++;
            }
            break;
        default: {
            uint32_t count = (*list).count;
            if (count >= (*sc).capRowCount)
                break; // region full (unreachable: one row per thread slot)
            uint32_t row = (*list).capBase + count;
            memcpy((*sc).caps + (size_t) row * stride, work, (size_t) stride * sizeof(size_t));
            RegexThread *slot = &(*list).threads[count];
            (*slot).pc = task.pc;
            (*slot).capRow = row;
            (*list).count = count + 1u;
            break;
        }
        }
    }
}

// Does a consuming instruction accept this byte? (CHAR bytes and class bitmaps
// were already folded at compile time.)
static bool instMatches(const Regex *re, const RegexInst *inst, uint8_t byte, bool dotall) {
    switch ((*inst).op) {
    case REGEX_OP_CHAR:
        return byte == (*inst).byte;
    case REGEX_OP_ANY:
        if (dotall)
            return true;
        return byte != (uint8_t) '\n';
    case REGEX_OP_CLASS: {
        const uint8_t *bitmap = (*re).classes + (size_t)(*inst).a * REGEX_CLASS_BYTES;
        return classTest(bitmap, byte);
    }
    default:
        return false;
    }
}

// Runs the Pike-VM simulation. Returns true on a match and copies the winning
// capture row into the scratch's best row. anchored restricts the match to 0.
static bool regexRun(const Regex *re, const char *subject, size_t length, bool anchored,
                     RegexScratch *sc) {
    uint32_t n = (*re).instCount;
    bool insensitive = ((*re).flags & REGEX_CASE_INSENSITIVE) != 0;
    bool dotall = ((*re).flags & REGEX_DOTALL) != 0;
    bool multiline = ((*re).flags & REGEX_MULTILINE) != 0;

    RegexList clist;
    clist.threads = (*sc).clistThreads;
    clist.seen = (*sc).clistSeen;
    clist.count = 0;
    clist.capBase = 0;
    RegexList nlist;
    nlist.threads = (*sc).nlistThreads;
    nlist.seen = (*sc).nlistSeen;
    nlist.count = 0;
    nlist.capBase = n;
    memset(clist.seen, 0, (size_t) n * sizeof(uint32_t));
    memset(nlist.seen, 0, (size_t) n * sizeof(uint32_t));

    bool matched = false;
    for (size_t pos = 0;; pos++) {
        if (clist.count == 0 && matched)
            break;
        if (!matched && (!anchored || pos == 0) && pos <= length)
            addThread(re, sc, &clist, 0, (*sc).seed, pos, subject, length, multiline);
        nlist.count = 0;
        memset(nlist.seen, 0, (size_t) n * sizeof(uint32_t));
        for (uint32_t i = 0; i < clist.count; i++) {
            RegexThread *t = &clist.threads[i];
            RegexInst *inst = &(*re).program[(*t).pc];
            if ((*inst).op == REGEX_OP_MATCH) {
                matched = true;
                memcpy((*sc).best, (*sc).caps + (size_t)(*t).capRow * (*sc).capStride,
                       (size_t)(*sc).capStride * sizeof(size_t));
                break;
            }
            if (pos < length) {
                uint8_t byte = (uint8_t) subject[pos];
                uint8_t folded = insensitive ? lowerByte(byte) : byte;
                if (instMatches(re, inst, folded, dotall))
                    addThread(re, sc, &nlist, (*t).pc + 1u,
                              (*sc).caps + (size_t)(*t).capRow * (*sc).capStride,
                              pos + 1u, subject, length, multiline);
            }
        }
        RegexList swap = clist;
        clist = nlist;
        nlist = swap;
        if (pos >= length)
            break;
    }
    return matched;
}

// ===========================================================================
// Text projection helpers (the toString Law)
// ===========================================================================

static void textAppend(RegexText *t, const char *text) {
    size_t len = strlen(text);
    if ((*t).used + len >= (*t).cap) {
        size_t room = (*t).cap > (*t).used + 1u ? (*t).cap - (*t).used - 1u : 0;
        if (room > 0)
            memcpy((*t).dest + (*t).used, text, room);
        (*t).used += room;
        (*t).dest[(*t).used] = '\0';
        (*t).truncated = true;
        return;
    }
    memcpy((*t).dest + (*t).used, text, len);
    (*t).used += len;
    (*t).dest[(*t).used] = '\0';
}

static void textPrintf(RegexText *t, const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n < 0)
        return;
    textAppend(t, buffer);
}

// Writes a fixed string into a caller buffer with truncation reporting.
static bool copyText(const char *text, char *dest, size_t cap, bool *outTruncated) {
    if (!dest || cap == 0)
        return false;
    size_t len = strlen(text);
    size_t room = cap - 1u;
    if (len > room) {
        memcpy(dest, text, room);
        dest[room] = '\0';
        if (outTruncated)
            *outTruncated = true;
        return false;
    }
    memcpy(dest, text, len);
    dest[len] = '\0';
    if (outTruncated)
        *outTruncated = false;
    return true;
}

static void freeInternals(Regex *re) {
    if ((*re).program) Memory_free((*re).program);
    if ((*re).classes) Memory_free((*re).classes);
    if ((*re).pattern) Memory_free((*re).pattern);
    (*re).program = nullptr;
    (*re).classes = nullptr;
    (*re).pattern = nullptr;
}

// ===========================================================================
// Public surface
// ===========================================================================

Regex *Regex_2(const char *pattern, uint32_t flags) {
    if (!pattern) {
        THROW("regex compile: null pattern");
        return nullptr;
    }
    Regex *re = (Regex*) Memory_alloc(TYPE_REGEX, sizeof(Regex));
    if (!re) {
        THROW("regex compile: out of memory");
        return nullptr;
    }
    memset(re, 0, sizeof(Regex));
    size_t length = strlen(pattern);
    (*re).pattern = (char*) regexGrow(nullptr, length + 1u);
    if (!(*re).pattern) {
        THROW("regex compile: out of memory");
        Memory_free(re);
        return nullptr;
    }
    memcpy((*re).pattern, pattern, length + 1u);
    (*re).flags = flags;

    RegexParser parserState;
    RegexParser *parser = &parserState;
    memset(parser, 0, sizeof(RegexParser));
    (*parser).pattern = pattern;
    (*parser).length = length;
    (*parser).regex = re;

    uint32_t root = parseAlternation(parser);
    bool ok = root != REGEX_NODE_NONE && !(*parser).failed;
    if (ok && (*parser).pos != length) {
        parserFail(parser, "unexpected ')'");
        ok = false;
    }
    (*re).groupCount = (*parser).groupCount;

    if (ok) {
        bool emitted = emit(re, REGEX_OP_SAVE, 0, 0, 0) != UINT32_MAX;
        if (emitted)
            emitted = emitNode(re, (*parser).nodes, root);
        if (emitted)
            emitted = emit(re, REGEX_OP_SAVE, 0, 1, 0) != UINT32_MAX;
        if (emitted)
            emitted = emit(re, REGEX_OP_MATCH, 0, 0, 0) != UINT32_MAX;
        if (!emitted)
            parserFail(parser, "program too large (program ceiling)");
        ok = emitted && !(*parser).failed;
    }

    if ((*parser).nodes)
        Memory_free((*parser).nodes);

    if (!ok) {
        THROW("regex compile: %s: %s", pattern, (*parser).error ? (*parser).error : "out of memory");
        freeInternals(re);
        Memory_free(re);
        return nullptr;
    }
    return re;
}

Regex *Regex_1(const char *pattern) {
    return Regex_2(pattern, REGEX_DEFAULT);
}

void Regex_free(Regex *regex) {
    if (!regex)
        return;
    freeInternals(regex);
    Memory_free(regex);
}

const char *Regex_pattern(const Regex *regex) {
    if (!regex)
        return "";
    return (*regex).pattern;
}

uint32_t Regex_flags(const Regex *regex) {
    if (!regex)
        return 0;
    return (*regex).flags;
}

uint32_t Regex_groupCount(const Regex *regex) {
    if (!regex)
        return 0;
    return (*regex).groupCount;
}

uint32_t Regex_instCount(const Regex *regex) {
    if (!regex)
        return 0;
    return (*regex).instCount;
}

uint32_t Regex_classCount(const Regex *regex) {
    if (!regex)
        return 0;
    return (*regex).classCount;
}

bool Regex_isMatch(const Regex *regex, const char *subject) {
    if (!regex || !subject)
        return false;
    RegexSpan span;
    return Regex_find(regex, subject, strlen(subject), &span);
}

bool Regex_isFullMatch(const Regex *regex, const char *subject) {
    if (!regex || !subject)
        return false;
    size_t length = strlen(subject);
    RegexScratch sc;
    memset(&sc, 0, sizeof(sc));
    if (!scratchInit(&sc, regex))
        return false;
    bool hit = regexRun(regex, subject, length, true, &sc);
    bool full = false;
    if (hit)
        full = sc.best[0] == 0 && sc.best[1] == length;
    scratchFree(&sc);
    return full;
}

bool Regex_findGroups(const Regex *regex, const char *subject, size_t length,
                      RegexSpan *outSpans, size_t spanCapacity, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!regex || !subject || !outSpans)
        return false;
    RegexScratch sc;
    memset(&sc, 0, sizeof(sc));
    if (!scratchInit(&sc, regex))
        return false;
    bool hit = regexRun(regex, subject, length, false, &sc);
    if (hit) {
        size_t *caps = sc.best;
        size_t groups = (size_t)(*regex).groupCount + 1u;
        size_t fill = groups < spanCapacity ? groups : spanCapacity;
        for (size_t g = 0; g < fill; g++) {
            outSpans[g].start = caps[2u * g];
            outSpans[g].end = caps[2u * g + 1u];
        }
        if (outTruncated && groups > spanCapacity)
            *outTruncated = true;
    }
    scratchFree(&sc);
    return hit;
}

bool Regex_find(const Regex *regex, const char *subject, size_t length, RegexSpan *outMatch) {
    if (!outMatch)
        return false;
    RegexSpan spans[1];
    if (!Regex_findGroups(regex, subject, length, spans, 1, nullptr))
        return false;
    (*outMatch).start = spans[0].start;
    (*outMatch).end = spans[0].end;
    return true;
}

bool Regex_toString(const Regex *regex, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!dest || cap == 0)
        return false;
    if (!regex)
        return copyText("nullptr", dest, cap, outTruncated);
    RegexText text;
    text.dest = dest;
    text.cap = cap;
    text.used = 0;
    text.truncated = false;
    dest[0] = '\0';
    textAppend(&text, "/");
    textAppend(&text, (*regex).pattern);
    textAppend(&text, "/");
    if ((*regex).flags & REGEX_CASE_INSENSITIVE) textAppend(&text, "i");
    if ((*regex).flags & REGEX_DOTALL) textAppend(&text, "s");
    if ((*regex).flags & REGEX_MULTILINE) textAppend(&text, "m");
    textPrintf(&text, " (groups=%u, insts=%u)", (*regex).groupCount, (*regex).instCount);
    if (outTruncated)
        *outTruncated = text.truncated;
    return !text.truncated;
}

bool Regex_toStringStruct(const Regex *regex, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!dest || cap == 0)
        return false;
    if (!regex)
        return copyText("nullptr", dest, cap, outTruncated);
    RegexText text;
    text.dest = dest;
    text.cap = cap;
    text.used = 0;
    text.truncated = false;
    dest[0] = '\0';
    textAppend(&text, "Regex { ");
    textPrintf(&text, "pattern=\"");
    // pattern text is emitted through textAppend so it is length-bounded.
    textAppend(&text, (*regex).pattern);
    textAppend(&text, "\", flags=");
    textPrintf(&text, "%u", (*regex).flags);
    textPrintf(&text, ", groupCount=%u", (*regex).groupCount);
    textPrintf(&text, ", instCount=%u", (*regex).instCount);
    textPrintf(&text, ", instCapacity=%u", (*regex).instCapacity);
    textPrintf(&text, ", classCount=%u", (*regex).classCount);
    textPrintf(&text, ", classCapacity=%u", (*regex).classCapacity);
    textAppend(&text, ", program=");
    textPrintf(&text, "%s", (*regex).program ? "set" : "null");
    textAppend(&text, ", classes=");
    textPrintf(&text, "%s", (*regex).classes ? "set" : "null");
    textAppend(&text, " }");
    if (outTruncated)
        *outTruncated = text.truncated;
    return !text.truncated;
}
