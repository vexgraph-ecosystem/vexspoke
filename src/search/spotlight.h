#ifndef SEARCH_SPOTLIGHT_H
#define SEARCH_SPOTLIGHT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// search/spotlight.h — Ranked Spotlight Search & Math Query Engine.
//
// Single Class Per File Law: Spotlight.
//
// Provides ranked candidate matching (Exact > Prefix > Word Boundary > Fuzzy)
// and handles calculator spotlight queries.

typedef struct SpotlightMatch {
    uint32_t id;
    int32_t score; // higher is better
} SpotlightMatch;

// Ranks candidate strings against query. Fills outMatches up to maxCount.
// Returns total number of matches found.
size_t Spotlight_rank(const char *query, const char *const *candidates, const uint32_t *ids,
                      size_t candidateCount, SpotlightMatch *outMatches, size_t maxCount);

// Evaluates whether a query string represents a math expression and computes its result.
bool Spotlight_tryCalculate(const char *query, double *outResult);

#endif
