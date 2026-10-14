#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "regex/charclass.h"
#include "regex/nodelist.h"

// regex/parser.h — the RegexParser: pattern bytes in, AST plus class bitmaps
// out. Single Class Per File Law: RegexParser.
//
// Recursive descent with the usual precedence: alternation < concatenation <
// repetition < atom. It owns the node list and the class-bitmap array it
// fills; the public compile entry (regex/regex.c) reads them, takes the
// classes, and frees the parser. A parse either produces a complete tree or
// records exactly one rejection reason and produces nothing usable — never a
// half-linked AST.
//
// Compile-time case folding is decided here (the caller passes ignoreCase),
// so the compiled program and its bitmaps already carry both cases and the
// match-time code never folds.

// Documented safety ceilings, owned here because this unit enforces them.
// Each is a refusal with a stated reason, never a silent truncation.
#define REGEX_MAX_NODES   8192u   // AST nodes (also bounds the emitter's recursion)
#define REGEX_MAX_CLASSES 4096u   // class bitmaps in one pattern
#define REGEX_MAX_DEPTH   128u    // group nesting
#define REGEX_MAX_REPEAT  1000u   // an explicit {m,n} bound

typedef struct RegexParser {
    const char     *pattern;       // borrowed pattern text (not owned)
    size_t          length;        // pattern byte length
    size_t          pos;           // parse cursor
    bool            ignoreCase;    // fold ASCII case while building the AST
    RegexNodeList   nodes;         // the AST being built (owned)
    RegexCharClass *classes;       // growable class bitmaps (owned)
    uint32_t        classCount;    // classes in use
    uint32_t        classCapacity; // classes allocated
    uint32_t        groupCount;    // capturing groups seen
    uint32_t        depth;         // current group nesting depth
    bool            failed;        // a rejection was recorded
    const char     *error;         // first rejection reason (static text)
} RegexParser;

// Prepare a parser over a borrowed pattern. Allocates nothing.
void RegexParser_init(RegexParser *parser, const char *pattern, size_t length, bool ignoreCase);

// Parse the whole pattern. Returns the root node index, or REGEX_NODE_NONE
// when the pattern is malformed or a ceiling was reached.
uint32_t RegexParser_parse(RegexParser *parser);

// True when the last parse completed without a rejection.
bool RegexParser_ok(const RegexParser *parser);

// The rejection reason, or nullptr when the parse was clean.
const char *RegexParser_error(const RegexParser *parser);

// The parsed AST (borrowed; valid until RegexParser_free).
RegexNodeList *RegexParser_nodes(RegexParser *parser);

// Capturing groups found (group 0 is the implicit whole match).
uint32_t RegexParser_groupCount(const RegexParser *parser);

// Transfer the class-bitmap array to the caller and write its count. The
// parser forgets it, so RegexParser_free will not release it.
RegexCharClass *RegexParser_releaseClasses(RegexParser *parser, uint32_t *outCount);

// Release the AST and any un-transferred class array. Null-safe.
void RegexParser_free(RegexParser *parser);

#endif
