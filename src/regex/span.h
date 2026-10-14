#ifndef REGEX_SPAN_H
#define REGEX_SPAN_H

#include <stdbool.h>
#include <stddef.h>

// regex/span.h — the RegexSpan value: a (start, end) byte-offset pair reported
// by a match. Single Class Per File Law: RegexSpan, a stateless value with
// three pure operations — no allocation, no engine-arena block, no lifetime.

// A span that did not participate (an unmatched capture group).
#define REGEX_UNSET ((size_t) -1)

// A half-open byte range inside one subject. end is exclusive.
typedef struct RegexSpan {
    size_t start;  // inclusive byte offset within the subject
    size_t end;    // exclusive byte offset
} RegexSpan;

// The "did not participate" span (both ends REGEX_UNSET).
RegexSpan RegexSpan_none(void);

// True when both ends are set, i.e. the group took part in the match.
bool RegexSpan_isSet(RegexSpan span);

// end - start for a set, non-inverted span; 0 otherwise.
size_t RegexSpan_length(RegexSpan span);

#endif
