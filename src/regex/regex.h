#ifndef REGEX_REGEX_H
#define REGEX_REGEX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "regex/charclass.h"
#include "regex/program.h"
#include "regex/span.h"

// regex/regex.h — the Regex class: a compiled, byte-oriented regular
// expression. Single Class Per File Law: Regex.
//
// Regex_2 is the public entry: it parses the pattern (regex/parser.c), emits a
// Thompson-NFA program (regex/program.c) and keeps the class bitmaps, then
// throws the AST away. Matching runs a Pike-VM simulation (regex/vm.c), which
// is linear in subject length for a fixed program, with capture-copy cost
// proportional to group count. No wall-clock budget/cancellation is offered.
// Case folding is resolved at compile
// time, so the match loop is a plain byte compare.
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
// be shared read-only across threads. Each match call builds its own bounded
// scratch proportional to the compiled program, so concurrent matching on one
// Regex is safe; matching is a cold path, never a frame path.

// Compile flags (Regex_2).
#define REGEX_DEFAULT          0u
#define REGEX_CASE_INSENSITIVE (1u << 0)  // ASCII case folding, compile-time
#define REGEX_DOTALL           (1u << 1)  // '.' also matches '\n'
#define REGEX_MULTILINE        (1u << 2)  // '^' and '$' also match line edges

// The compiled regular expression. After Regex_2 returns it is immutable and
// safe to share read-only; Regex_free releases the block, its program and its
// class bitmaps.
typedef struct Regex {
    char           *pattern;     // owned copy of the source pattern text
    uint32_t        flags;       // REGEX_* options used at compile time
    uint32_t        groupCount;  // capturing groups (group 0 is the whole match)
    RegexProgram    program;     // the compiled instruction stream
    RegexCharClass *classes;     // classCount bitmaps (engine-arena block)
    uint32_t        classCount;  // class bitmaps in use
} Regex;

// Compile with REGEX_DEFAULT flags. Returns nullptr (loudly) on a bad pattern.
Regex *Regex_1(const char *pattern);

// Compile with explicit REGEX_* flags. Returns nullptr (loudly) on a bad
// pattern; a null pattern is rejected the same way.
Regex *Regex_2(const char *pattern, uint32_t flags);
// Unsupported alphabetic escapes/backreferences and repeated quantifiers
// reject, including lazy/possessive spellings; no literal fallback is offered.

// Release the compiled pattern, its program and its class bitmaps. Null-safe.
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
// End admission occurs inside the VM: a|ab fully matches ab despite the
// shorter first branch. Ordinary search keeps first-alternative priority.
bool Regex_isFullMatch(const Regex *regex, const char *subject);

// Finds the first (leftmost) match in a byte span and writes its (start, end)
// into outMatch. Returns false, writing nothing, when there is no match.
bool Regex_find(const Regex *regex, const char *subject, size_t length,
                RegexSpan *outMatch);

// Finds the first match and fills outSpans[0..groupCount] (group 0 = the whole
// match). Writes at most spanCapacity spans; a group that did not participate
// is RegexSpan_none(). Sets *outTruncated (when non-null) if the subject has
// more groups than the buffer holds. Returns false, writing nothing, when
// there is no match (the Truncation-Never-Silent clause).
bool Regex_findGroups(const Regex *regex, const char *subject, size_t length,
                      RegexSpan *outSpans, size_t spanCapacity, bool *outTruncated);

// --- projections (the toString Law) ---

// "/pattern/flags" summary with the capture-group count; null-safe ("nullptr").
bool Regex_toString(const Regex *regex, char *dest, size_t cap, bool *outTruncated);

// A by-name dump of this class's own fields, one layer deep.
bool Regex_toStringStruct(const Regex *regex, char *dest, size_t cap, bool *outTruncated);

#endif
