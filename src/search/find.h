#ifndef SEARCH_FIND_H
#define SEARCH_FIND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// search/find.h — IDE-Style Find Flags & Pattern Matching.
//
// Single Class Per File Law: Find.
//
// Flexible string searching supporting bitmask flags:
//   - FIND_CASE_SENSITIVE: default is case-insensitive
//   - FIND_EXACT_WORD: match whole words bounded by whitespace/punctuation
//   - FIND_LIKE_WILDCARD: SQL-style '%' (0+ chars) and '_' (1 char)

#define FIND_DEFAULT         0u
#define FIND_CASE_SENSITIVE  (1u << 0)
#define FIND_EXACT_WORD      (1u << 1)
#define FIND_LIKE_WILDCARD   (1u << 2)

// Returns true if text matches pattern under the specified flags.
bool Search_match(const char *text, const char *pattern, uint32_t flags);

// Finds the 0-indexed offset of pattern within text, or -1 if not found.
int Search_findFirst(const char *text, const char *pattern, uint32_t flags);

// Convenience SQL LIKE matcher (supports '%' and '_')
bool Search_like(const char *text, const char *pattern, bool caseSensitive);

// Convenience exact whole-word matcher
bool Search_exactWord(const char *text, const char *word, bool caseSensitive);

#endif
