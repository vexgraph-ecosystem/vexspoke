// regex/regex.c — the public Regex class: compile once, match in linear time.

#include "regex/regex.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "exception/throw.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "regex/parser.h"
#include "regex/vm.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Regex
 * ============================================================================
 * The public face of the regex subsystem: compile a pattern once, then answer
 * "does this text look like that shape?" in time bounded by the input. It
 * exists because the ecosystem keeps needing text shapes — find/replace in
 * editors, log and manifest scanning, connector input validation, syntax
 * tooling — and neither a hand-rolled strstr nor a backtracking matcher is an
 * acceptable answer: the first cannot express the shape, the second hands a
 * hostile pattern a way to hang its caller.
 *
 * This unit owns only the orchestration and the public contract. The work
 * lives in one unit per concern: regex/parser.c turns pattern bytes into a
 * flat AST plus class bitmaps, regex/program.c lowers that AST into a Thompson
 * NFA, and regex/vm.c simulates it. Compiling is parse -> emit -> keep the
 * classes -> drop the AST; matching builds a fresh VM, runs it, reads spans and
 * frees it, so a compiled Regex is immutable and shareable while two threads
 * never share mutable state.
 *
 * memory: the Regex block, its pattern copy, its RegexProgram and its class
 * bitmap array all live in the engine arena and are stamped TYPE_REGEX. Each
 * match call allocates its own bounded scratch (in regex/vm.c) and releases it
 * on every path, including the no-match return.
 *
 * Failure: a null pattern, an unsupported escape or group, an unbalanced
 * bracket, an invalid range, a malformed quantifier or any safety-ceiling
 * breach is a loud cold rejection (THROW) returning nullptr — nothing
 * half-compiled escapes, because a half-compiled pattern would match the wrong
 * language (the Cold-Strict, Hot-Minimal Validation Law).
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Regex (regex/regex.c — defined in regex/regex.h)
 * ============================================================================
 * A compiled, byte-oriented regular expression: the public compile entry, the
 * public match surface, and the two string projections.
 *
 * STRUCT FIELDS (Mirroring regex/regex.h):
 * ----------------------------------------------------------------------------
 *   Regex {
 *     char           *pattern;     // owned copy of the source pattern text
 *     uint32_t        flags;       // REGEX_* options used at compile time
 *     uint32_t        groupCount;  // capturing groups (group 0 is the whole match)
 *     RegexProgram    program;     // the compiled instruction stream
 *     RegexCharClass *classes;     // classCount bitmaps (engine-arena block)
 *     uint32_t        classCount;  // class bitmaps in use
 *   }
 *
 * PRIVATE HELPERS:
 * ----------------------------------------------------------------------------
 *   RegexText { char *dest; size_t cap; size_t used; bool truncated; }
 *     Bounded-append cursor for the two string projections.
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
 *   - regexOptions(flags) / compilePattern(regex, pattern, length)
 *   - textAppend(text, s) / textPrintf(text, fmt, ...) / copyText(s, dest, cap, flag)
 *
 * Admission, ownership, failure:
 *   - Regex_1/Regex_2 admit a non-null NUL-terminated pattern; every rejection
 *     is loud (THROW) and returns nullptr with nothing partially published.
 *   - Regex_find / Regex_findGroups / Regex_isMatch / Regex_isFullMatch write
 *     outputs only on a match; on no match they return false with outputs
 *     untouched.
 *   - Regex_free is null-safe and releases pattern, program and classes.
 *   - Matching never allocates outside the VM's own scratch and always frees
 *     it, including on a no-match return.
 * ============================================================================
 */

typedef struct RegexText {
    char  *dest;       // caller buffer
    size_t cap;        // caller capacity (always > 0 here)
    size_t used;       // bytes written, excluding the NUL
    bool   truncated;  // true once a write did not fit
} RegexText;

// Maps the public flag word onto the VM's resolved options.
static RegexOptions regexOptions(uint32_t flags) {
    RegexOptions options;
    options.ignoreCase = (flags & REGEX_CASE_INSENSITIVE) != 0;
    options.dotall = (flags & REGEX_DOTALL) != 0;
    options.multiline = (flags & REGEX_MULTILINE) != 0;
    return options;
}

// Runs one anchored or unanchored search and, on success, reports spans.
static bool runSearch(const Regex *regex, const char *subject, size_t length,
                      bool anchored, RegexSpan *outSpans, size_t spanCapacity,
                      size_t *outGroupCount, bool *outTruncated) {
    if (!regex || !subject)
        return false;
    RegexVm vm;
    if (!RegexVm_init(&vm, &(*regex).program, (*regex).groupCount))
        return false;
    bool hit = RegexVm_run(&vm, (*regex).classes, subject, length, anchored,
                           regexOptions((*regex).flags));
    if (hit && outSpans) {
        size_t groups = (size_t)(*regex).groupCount + 1u;
        size_t fill = groups < spanCapacity ? groups : spanCapacity;
        for (size_t g = 0; g < fill; g++)
            outSpans[g] = RegexVm_span(&vm, (uint32_t) g);
        if (outTruncated && groups > spanCapacity)
            *outTruncated = true;
    }
    if (hit && outGroupCount)
        *outGroupCount = (size_t)(*regex).groupCount + 1u;
    RegexVm_free(&vm);
    return hit;
}

// Appends a NUL-terminated string, flagging a truncation instead of overrunning.
static void textAppend(RegexText *text, const char *s) {
    size_t len = strlen(s);
    if ((*text).used + len >= (*text).cap) {
        size_t room = (*text).cap > (*text).used + 1u ? (*text).cap - (*text).used - 1u : 0;
        if (room > 0)
            memcpy((*text).dest + (*text).used, s, room);
        (*text).used += room;
        (*text).dest[(*text).used] = '\0';
        (*text).truncated = true;
        return;
    }
    memcpy((*text).dest + (*text).used, s, len);
    (*text).used += len;
    (*text).dest[(*text).used] = '\0';
}

static void textPrintf(RegexText *text, const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (n < 0)
        return;
    textAppend(text, buffer);
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

// Parse, emit and adopt for one already-allocated Regex.
static bool compilePattern(Regex *regex, const char *pattern, size_t length) {
    RegexParser parser;
    RegexParser_init(&parser, pattern, length,
                     ((*regex).flags & REGEX_CASE_INSENSITIVE) != 0);
    uint32_t root = RegexParser_parse(&parser);
    bool ok = root != REGEX_NODE_NONE && RegexParser_ok(&parser);
    if (ok) {
        (*regex).groupCount = RegexParser_groupCount(&parser);
        ok = RegexProgram_build(&(*regex).program, RegexParser_nodes(&parser), root);
    }
    if (ok) {
        uint32_t classCount = 0;
        RegexCharClass *classes = RegexParser_releaseClasses(&parser, &classCount);
        (*regex).classes = classes;
        (*regex).classCount = classCount;
    }
    const char *error = RegexParser_error(&parser);
    RegexParser_free(&parser);
    if (!ok && error)
        THROW("regex compile: %s: %s", pattern, error);
    else if (!ok)
        THROW("regex compile: %s: program too large (program ceiling)", pattern);
    return ok;
}

Regex *Regex_2(const char *pattern, uint32_t flags) {
    if (!pattern) {
        THROW("regex compile: null pattern");
        return nullptr;
    }
    Regex *regex = (Regex*) Memory_alloc(TYPE_REGEX, sizeof(Regex));
    if (!regex) {
        THROW("regex compile: out of memory");
        return nullptr;
    }
    memset(regex, 0, sizeof(Regex));
    RegexProgram_init(&(*regex).program);
    (*regex).flags = flags;

    size_t length = strlen(pattern);
    (*regex).pattern = (char*) Memory_alloc(TYPE_REGEX, length + 1u);
    if (!(*regex).pattern) {
        THROW("regex compile: out of memory");
        Memory_free(regex);
        return nullptr;
    }
    memcpy((*regex).pattern, pattern, length + 1u);

    if (!compilePattern(regex, pattern, length)) {
        Regex_free(regex);
        return nullptr;
    }
    return regex;
}

Regex *Regex_1(const char *pattern) {
    return Regex_2(pattern, REGEX_DEFAULT);
}

void Regex_free(Regex *regex) {
    if (!regex)
        return;
    RegexProgram_free(&(*regex).program);
    if ((*regex).classes)
        Memory_free((*regex).classes);
    if ((*regex).pattern)
        Memory_free((*regex).pattern);
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
    return RegexProgram_count(&(*regex).program);
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
    return runSearch(regex, subject, strlen(subject), false, &span, 1u, nullptr, nullptr);
}

bool Regex_isFullMatch(const Regex *regex, const char *subject) {
    if (!regex || !subject)
        return false;
    size_t length = strlen(subject);
    RegexSpan whole = RegexSpan_none();
    if (!runSearch(regex, subject, length, true, &whole, 1u, nullptr, nullptr))
        return false;
    return whole.start == 0 && whole.end == length;
}

bool Regex_find(const Regex *regex, const char *subject, size_t length, RegexSpan *outMatch) {
    if (!outMatch || !regex || !subject)
        return false;
    RegexSpan span = RegexSpan_none();
    if (!runSearch(regex, subject, length, false, &span, 1u, nullptr, nullptr))
        return false;
    (*outMatch).start = span.start;
    (*outMatch).end = span.end;
    return true;
}

bool Regex_findGroups(const Regex *regex, const char *subject, size_t length,
                      RegexSpan *outSpans, size_t spanCapacity, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!regex || !subject || !outSpans)
        return false;
    return runSearch(regex, subject, length, false, outSpans, spanCapacity, nullptr, outTruncated);
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
    textPrintf(&text, " (groups=%u, insts=%u)", (*regex).groupCount, Regex_instCount(regex));
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
    textAppend(&text, "Regex { pattern=\"");
    textAppend(&text, (*regex).pattern);
    textAppend(&text, "\", flags=");
    textPrintf(&text, "%u", (*regex).flags);
    textPrintf(&text, ", groupCount=%u", (*regex).groupCount);
    textPrintf(&text, ", instCount=%u", Regex_instCount(regex));
    textPrintf(&text, ", classCount=%u", (*regex).classCount);
    textAppend(&text, ", program=");
    textPrintf(&text, "%s", (*regex).program.insts ? "set" : "null");
    textAppend(&text, ", classes=");
    textPrintf(&text, "%s", (*regex).classes ? "set" : "null");
    textAppend(&text, " }");
    if (outTruncated)
        *outTruncated = text.truncated;
    return !text.truncated;
}
