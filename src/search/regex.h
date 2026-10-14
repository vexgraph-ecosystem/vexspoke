#ifndef SEARCH_REGEX_H
#define SEARCH_REGEX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"

// search/regex.h — the Regex class: a compiled, byte-oriented regular
// expression (Single Class Per File Law: Regex).
//
// Regex_2 compiles a pattern once into a flat Thompson-NFA program plus a set
// of 256-bit character-class bitmaps, all owned by the Regex block in the
// engine arena. Matching then runs a Pike-VM simulation over that program:
// linear in subject length x program length, with no backtracking, so a
// hostile pattern cannot make one match hang the caller. Folding for
// REGEX_CASE_INSENSITIVE happens at compile time (pattern bytes and class
// bitmaps), so the match loop is a plain byte compare.
//
// Matching is BYTE-oriented, not Unicode: the subject is a byte span and every
// construct means bytes. Capture groups are reported as (start, end) byte
// offsets, group 0 being the whole match; an unmatched group is REGEX_UNSET.
//
// Supported syntax:
//   literals                     a b c
//   any byte (except LF)         .
//   anchors                      ^  $          (line anchors under REGEX_MULTILINE)
//   alternation                  a|b
//   quantifiers (greedy)         *  +  ?  {m}  {m,}  {m,n}
//   groups                       (...)  (?:...)
//   classes                      [abc]  [^abc]  [a-z]  [\d\w]
//   escapes                      \n \r \t \f \v \0 \xHH  \d \D \w \W \s \S
//                                and any escaped metacharacter (\* \. \\ ...)
//
// Not in this version (explicit gaps, not silent): lazy quantifiers (*?, +?),
// possessive quantifiers, lookaround, backreferences, \b/\B word boundaries,
// named groups, and Unicode character classes. An unsupported escape or a
// malformed construct is rejected loudly at compile time (the THROW Law);
// Regex_2 returns nullptr and the offending pattern is never half-compiled.
//
// Thread contract: a compiled Regex is immutable after Regex_2 returns and may
// be shared read-only across threads. Each match call allocates its own bounded
// scratch proportional to the compiled program, so concurrent matching on one
// Regex is safe; matching is a cold path, never a frame path (the Cold-Only
// Reflection Law's spirit — resolve once at compile, reuse the program).

// Compile flags (Regex_2).
#define REGEX_DEFAULT          0u
#define REGEX_CASE_INSENSITIVE (1u << 0)  // ASCII case folding, compile-time
#define REGEX_DOTALL           (1u << 1)  // '.' also matches '\n'
#define REGEX_MULTILINE        (1u << 2)  // '^' and '$' also match line edges

// One character-class bitmap: 256 bits, one bit per byte value.
#define REGEX_CLASS_BYTES 32u

// A span that did not participate (an unmatched capture group).
#define REGEX_UNSET ((size_t) -1)

// Compiled instruction opcodes (private vocabulary surfaced for introspection
// and tests; consumers normally never switch on these).
typedef enum RegexOp {
    REGEX_OP_CHAR = 0,  // consume (*inst).byte
    REGEX_OP_ANY,       // consume any byte (respecting REGEX_DOTALL)
    REGEX_OP_CLASS,     // consume a byte in class bitmap (*inst).a
    REGEX_OP_SPLIT,     // fork to a then b (priority order)
    REGEX_OP_JUMP,      // continue at a
    REGEX_OP_SAVE,      // record subject offset into capture slot a
    REGEX_OP_BOL,       // zero-width: start of subject / line
    REGEX_OP_EOL,       // zero-width: end of subject / line
    REGEX_OP_MATCH      // the program accepted
} RegexOp;

// One compiled instruction. a/b are instruction indices, capture slots, or
// class indices depending on op.
typedef struct RegexInst {
    uint8_t  op;        // RegexOp
    uint8_t  byte;      // literal byte for REGEX_OP_CHAR (already folded)
    uint16_t reserved;  // keeps the record 12 Bytes and naturally aligned
    uint32_t a;         // first operand (target / class index / capture slot)
    uint32_t b;         // second operand (alternation branch target)
} RegexInst;

// A (start, end) byte-offset pair; end is exclusive. See REGEX_UNSET.
typedef struct RegexSpan {
    size_t start;
    size_t end;
} RegexSpan;

// The compiled regular expression. After Regex_2 returns it is immutable and
// safe to share read-only; Regex_free releases the block and its program.
typedef struct Regex {
    char      *pattern;        // owned copy of the source pattern text
    uint32_t   flags;          // REGEX_* options used at compile time
    uint32_t   groupCount;     // capturing groups (group 0 is the whole match)
    uint32_t   instCount;      // compiled instructions in use
    uint32_t   instCapacity;   // allocated instruction slots
    uint32_t   classCount;     // character-class bitmaps in use
    uint32_t   classCapacity;  // allocated class slots
    RegexInst *program;        // the compiled instruction stream
    uint8_t   *classes;        // classCount * REGEX_CLASS_BYTES bitmaps
} Regex;

// Compile with REGEX_DEFAULT flags. Returns nullptr (loudly) on a bad pattern.
Regex *Regex_1(const char *pattern);

// Compile with explicit REGEX_* flags. Returns nullptr (loudly) on a bad
// pattern; a null pattern is rejected the same way.
Regex *Regex_2(const char *pattern, uint32_t flags);

// Release the compiled pattern and its program. Null-safe.
void Regex_free(Regex *regex);

#define Regex(...) CONSTRUCTOR_DISPATCH(Regex, __VA_ARGS__)

// --- introspection (cheap reads of the compiled form) ---

// The original pattern text (never null for a live Regex).
const char *Regex_pattern(const Regex *regex);

// The REGEX_* flags this Regex was compiled with.
uint32_t Regex_flags(const Regex *regex);

// Capturing groups, excluding the whole-match group 0.
uint32_t Regex_groupCount(const Regex *regex);

// Compiled instruction count.
uint32_t Regex_instCount(const Regex *regex);

// Character-class bitmap count.
uint32_t Regex_classCount(const Regex *regex);

// --- matching (byte-oriented, allocation-bounded scratch per call) ---

// True when the pattern matches anywhere in the NUL-terminated subject.
bool Regex_isMatch(const Regex *regex, const char *subject);

// True when the pattern matches and consumes the entire NUL-terminated subject.
bool Regex_isFullMatch(const Regex *regex, const char *subject);

// Finds the first (leftmost) match in a byte span and writes its (start, end)
// into outMatch. Returns false, writing nothing, when there is no match.
bool Regex_find(const Regex *regex, const char *subject, size_t length,
                RegexSpan *outMatch);

// Finds the first match and fills outSpans[0..groupCount] (group 0 = the whole
// match). Writes at most spanCapacity spans; a group that did not participate
// gets start == end == REGEX_UNSET. Sets *outTruncated (when non-null) if the
// subject has more groups than the buffer holds. Returns false, writing
// nothing, when there is no match (the Truncation-Never-Silent clause).
bool Regex_findGroups(const Regex *regex, const char *subject, size_t length,
                      RegexSpan *outSpans, size_t spanCapacity, bool *outTruncated);

// --- projections (the toString Law) ---

// "/pattern/flags" summary with the capture-group count; null-safe ("nullptr").
bool Regex_toString(const Regex *regex, char *dest, size_t cap, bool *outTruncated);

// A by-name dump of this class's own fields, one layer deep.
bool Regex_toStringStruct(const Regex *regex, char *dest, size_t cap, bool *outTruncated);

#endif
