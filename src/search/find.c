#include "search/find.h"

#include <ctype.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Find
 * ============================================================================
 * Evaluates text searches with flags: case-sensitivity, whole-word boundaries,
 * and SQL '%' and '_' wildcards. Pure procedural — no state, no allocation;
 * every function walks caller-owned strings and returns a match index or
 * boolean. Private helpers implement the LIKE state machine, word-boundary
 * checks, and the substring scan with optional exact-word gating.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Find (search/find.c — defined in search/find.h)
 * ============================================================================
 * Evaluates text searches with flags: case-sensitivity, whole word boundaries,
 * and SQL '%' and '_' wildcards.
 *
 * STRUCT FIELDS: none — procedural (operates on caller-owned text/pattern
 * strings; no state)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - Search_match(text, pattern, flags)
 *   - Search_findFirst(text, pattern, flags)
 *   - Search_like(text, pattern, caseSensitive)
 *   - Search_exactWord(text, word, caseSensitive)
 *
 * Private Core Functions: (.c static)
 *   - normalize_char(c, caseSensitive)
 *   - match_like(t, p, caseSensitive)
 *   - is_word_boundary(c)
 *   - find_substring(text, pattern, caseSensitive, exactWord)
 * ============================================================================
 */

// Returns c unchanged or lowercased for case-insensitive matching.
static inline char normalize_char(char c, bool caseSensitive) {
    return caseSensitive ? c : (char) tolower((unsigned char) c);
}

// Matches a NUL-terminated string against SQL-style '%' and '_' wildcards.
static bool match_like(const char *t, const char *p, bool caseSensitive) {
    const char *text = t;
    const char *pattern = p;
    const char *last_star = nullptr;
    const char *last_match = nullptr;

    while (*text != '\0') {
        if (*pattern == '%') {
            last_star = pattern;
            pattern++;
            last_match = text;
        } else if (*pattern == '_' || normalize_char(*text, caseSensitive) == normalize_char(*pattern, caseSensitive)) {
            text++;
            pattern++;
        } else if (last_star != nullptr) {
            pattern = last_star + 1;
            last_match++;
            text = last_match;
        } else {
            return false;
        }
    }

    while (*pattern == '%') {
        pattern++;
    }

    return *pattern == '\0';
}

// Returns whether a character separates words for exact-word matching.
static bool is_word_boundary(char c) {
    return !isalnum((unsigned char) c) && c != '_';
}

// Finds the first substring match, optionally requiring word boundaries; returns -1 when absent.
static int find_substring(const char *text, const char *pattern, bool caseSensitive, bool exactWord) {
    if (!text || !pattern) return -1;
    size_t tlen = strlen(text);
    size_t plen = strlen(pattern);
    if (plen == 0) return 0;
    if (plen > tlen) return -1;

    for (size_t i = 0; i <= tlen - plen; i++) {
        bool match = true;
        for (size_t j = 0; j < plen; j++) {
            if (normalize_char(text[i + j], caseSensitive) != normalize_char(pattern[j], caseSensitive)) {
                match = false;
                break;
            }
        }
        if (match) {
            if (exactWord) {
                bool leftOk = (i == 0) || is_word_boundary(text[i - 1]);
                bool rightOk = (i + plen == tlen) || is_word_boundary(text[i + plen]);
                if (!leftOk || !rightOk) {
                    continue;
                }
            }
            return (int) i;
        }
    }
    return -1;
}

// Tests text using the requested case, whole-word, and wildcard flags.
bool Search_match(const char *text, const char *pattern, uint32_t flags) {
    if (!text || !pattern) return false;
    bool caseSensitive = (flags & FIND_CASE_SENSITIVE) != 0;
    bool exactWord = (flags & FIND_EXACT_WORD) != 0;
    bool isLike = (flags & FIND_LIKE_WILDCARD) != 0;

    if (isLike) {
        return match_like(text, pattern, caseSensitive);
    }
    if (exactWord) {
        return find_substring(text, pattern, caseSensitive, true) >= 0;
    }
    return find_substring(text, pattern, caseSensitive, false) >= 0;
}

// Returns the first matching byte offset, or -1 when inputs or match are absent.
int Search_findFirst(const char *text, const char *pattern, uint32_t flags) {
    if (!text || !pattern) return -1;
    bool caseSensitive = (flags & FIND_CASE_SENSITIVE) != 0;
    bool exactWord = (flags & FIND_EXACT_WORD) != 0;
    return find_substring(text, pattern, caseSensitive, exactWord);
}

// Tests text against a SQL-style wildcard pattern with optional case folding.
bool Search_like(const char *text, const char *pattern, bool caseSensitive) {
    if (!text || !pattern) return false;
    return match_like(text, pattern, caseSensitive);
}

// Tests whether word occurs as a complete word in text.
bool Search_exactWord(const char *text, const char *word, bool caseSensitive) {
    if (!text || !word) return false;
    return find_substring(text, word, caseSensitive, true) >= 0;
}
