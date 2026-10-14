#ifndef REGEX_CHARCLASS_H
#define REGEX_CHARCLASS_H

#include <stdbool.h>
#include <stdint.h>

// regex/charclass.h — the RegexCharClass: one 256-bit bitmap, one bit per byte
// value. Single Class Per File Law: RegexCharClass.
//
// A bracket expression and each of the \d \w \s families compile to one of
// these, so a match-time membership test is a single bit read — no range list,
// no ordering, no binary search. Case folding is applied when bits are added,
// so the bitmap already contains both cases and the matcher folds nothing.

// Bytes in one bitmap (256 bits).
#define REGEX_CLASS_BYTES 32u

typedef struct RegexCharClass {
    uint8_t bits[REGEX_CLASS_BYTES];  // one bit per byte value
} RegexCharClass;

// Zero every bit.
void RegexCharClass_clear(RegexCharClass *cls);

// Set one byte, and its opposite ASCII case when insensitive is true.
void RegexCharClass_addByte(RegexCharClass *cls, uint8_t byte, bool insensitive);

// Set an inclusive byte range, folding both cases when insensitive is true.
// lo > hi is refused (the caller validates ranges; this stays total).
void RegexCharClass_addRange(RegexCharClass *cls, uint8_t lo, uint8_t hi, bool insensitive);

// Union one of the \d \D \w \W \s \S families; an unknown letter adds nothing.
void RegexCharClass_addPredefined(RegexCharClass *cls, char letter);

// True when letter names a predefined family (\d \D \w \W \s \S).
bool RegexCharClass_isPredefined(char letter);

// Complement every bit.
void RegexCharClass_negate(RegexCharClass *cls);

// Test one byte.
bool RegexCharClass_test(const RegexCharClass *cls, uint8_t byte);

#endif
