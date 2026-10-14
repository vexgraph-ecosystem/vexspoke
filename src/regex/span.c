// regex/span.c — the RegexSpan value helpers.

#include "regex/span.h"

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: RegexSpan
 * ============================================================================
 * The (start, end) byte-offset pair a match reports, plus the three pure
 * operations a consumer needs to read it: build the "did not participate"
 * value, ask whether a group took part, and measure a span. It exists so the
 * public match surface never has to explain REGEX_UNSET at a call site, and so
 * an unmatched capture group is a value a caller can test rather than a magic
 * number to remember.
 *
 * memory: none. RegexSpan is a by-value record; these functions allocate
 * nothing, hold no state and are safe on any thread. Lifetime is the caller's.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: RegexSpan (regex/span.c — defined in regex/span.h)
 * ============================================================================
 * A stateless byte-offset pair; three pure predicates over it.
 *
 * STRUCT FIELDS (Mirroring regex/span.h):
 * ----------------------------------------------------------------------------
 *   RegexSpan {
 *     size_t start;  // inclusive byte offset within the subject
 *     size_t end;    // exclusive byte offset
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - RegexSpan_none()
 *   - RegexSpan_isSet(span)
 *   - RegexSpan_length(span)
 *
 * Private Core Functions: (.c static)
 *   - none
 *
 * Admission, ownership, failure:
 *   - Pure value operations: no allocation, no failure path, no state.
 *   - An unset span is (REGEX_UNSET, REGEX_UNSET); isSet is true only when
 *     both ends are set, and length is 0 for an unset or inverted span.
 * ============================================================================
 */

RegexSpan RegexSpan_none(void) {
    RegexSpan span;
    span.start = REGEX_UNSET;
    span.end = REGEX_UNSET;
    return span;
}

bool RegexSpan_isSet(RegexSpan span) {
    return span.start != REGEX_UNSET && span.end != REGEX_UNSET;
}

size_t RegexSpan_length(RegexSpan span) {
    if (!RegexSpan_isSet(span) || span.end < span.start)
        return 0;
    return span.end - span.start;
}
