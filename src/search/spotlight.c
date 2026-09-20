#include "search/spotlight.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "search/calc.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Spotlight
 * ============================================================================
 * Ranked search and math-query engine: scores candidate strings against a
 * query using tiered relevance (exact > prefix > word-boundary > substring >
 * fuzzy subsequence) and fills a caller-owned SpotlightMatch array, sorted
 * descending by score. Also evaluates in-line calculator queries by
 * delegating to Calc_eval. Pure procedural — no state, no allocation beyond
 * the caller-provided outMatches buffer.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Spotlight (search/spotlight.c — defined in search/spotlight.h)
 * LEVEL: L2 — Behavior (ranked search and math expression evaluation)
 * ============================================================================
 * Scores query strings against candidates using tiered relevance ranking and
 * evaluates in-line mathematical calculations.
 *
 * STRUCT FIELDS (Mirroring search/spotlight.h):
 * ----------------------------------------------------------------------------
 *   SpotlightMatch {
 *     uint32_t id;    // candidate index (or caller-supplied id)
 *     int32_t score;  // relevance score, higher is better
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - Spotlight_rank(query, candidates, ids, candidateCount, outMatches, maxCount)
 *   - Spotlight_tryCalculate(query, outResult)
 *
 * Private Core Functions: (.c static)
 *   - calculate_score(candidate, query)
 *   - compare_matches(a, b)
 * ============================================================================
 */

static int calculate_score(const char *candidate, const char *query) {
    if (!candidate || !query) return 0;
    size_t qlen = strlen(query);
    size_t clen = strlen(candidate);
    if (qlen == 0) return 0;

    // 1. Exact match
    if (strcasecmp(candidate, query) == 0) {
        return 1000;
    }

    // 2. Prefix match
    if (strncasecmp(candidate, query, qlen) == 0) {
        return 800 - (int)(clen - qlen); // shorter candidates ranked higher
    }

    // 3. Word-boundary prefix match (e.g. "bar" matching "foo_bar")
    for (size_t i = 1; i < clen; i++) {
        if ((candidate[i - 1] == '_' || candidate[i - 1] == '-' || isupper((unsigned char)candidate[i])) &&
            strncasecmp(candidate + i, query, qlen) == 0) {
            return 600 - (int)(clen - qlen);
        }
    }

    // 4. Substring match
    char qlower[64];
    char clower[128];
    size_t ql = qlen < sizeof(qlower) - 1 ? qlen : sizeof(qlower) - 1;
    for (size_t i = 0; i < ql; i++) qlower[i] = (char) tolower((unsigned char)query[i]);
    qlower[ql] = '\0';

    size_t cl = clen < sizeof(clower) - 1 ? clen : sizeof(clower) - 1;
    for (size_t i = 0; i < cl; i++) clower[i] = (char) tolower((unsigned char)candidate[i]);
    clower[cl] = '\0';

    if (strstr(clower, qlower) != NULL) {
        return 400 - (int)(clen - qlen);
    }

    // 5. Fuzzy subsequence match
    size_t qi = 0;
    for (size_t ci = 0; ci < cl && qi < ql; ci++) {
        if (clower[ci] == qlower[qi]) {
            qi++;
        }
    }
    if (qi == ql) {
        return 200 - (int)(clen - qlen);
    }

    return 0;
}

static int compare_matches(const void *a, const void *b) {
    const SpotlightMatch *ma = (const SpotlightMatch*) a;
    const SpotlightMatch *mb = (const SpotlightMatch*) b;
    return (*mb).score - (*ma).score; // descending
}

size_t Spotlight_rank(const char *query, const char *const *candidates, const uint32_t *ids,
                      size_t candidateCount, SpotlightMatch *outMatches, size_t maxCount) {
    if (!query || !candidates || !outMatches || maxCount == 0) return 0;

    size_t matchCount = 0;
    for (size_t i = 0; i < candidateCount; i++) {
        int score = calculate_score(candidates[i], query);
        if (score > 0) {
            if (matchCount < maxCount) {
                outMatches[matchCount].id = ids ? ids[i] : (uint32_t) i;
                outMatches[matchCount].score = score;
                matchCount++;
            }
        }
    }

    if (matchCount > 1) {
        qsort(outMatches, matchCount, sizeof(SpotlightMatch), compare_matches);
    }

    return matchCount;
}

bool Spotlight_tryCalculate(const char *query, double *outResult) {
    if (!query || !outResult) return false;
    return Calc_eval(query, outResult);
}
