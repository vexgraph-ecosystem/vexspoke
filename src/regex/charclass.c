// regex/charclass.c — the RegexCharClass 256-bit bitmap.

#include "regex/charclass.h"

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: RegexCharClass
 * ============================================================================
 * The byte-membership set used by bracket expressions and the \d \w \s
 * families. One 256-bit bitmap makes both compilation and matching trivial:
 * building a class is a bit set per member, and testing one at match time is a
 * shift, a mask and a load — no range list to walk and no ordering to keep, so
 * a class costs the same whether it names one byte or two hundred.
 *
 * Case folding happens where the bits are added: with an insensitive compile,
 * every letter sets both of its cases, so the matcher never folds the subject.
 * That keeps the hot path a plain bit read (the Cold-Strict, Hot-Minimal
 * Validation Law's split: decide once at compile, trust the bitmap at match).
 *
 * memory: none. A RegexCharClass is a 32-byte by-value record; the class ARRAY
 * a compiled pattern owns is grown by the parser (regex/parser.c). Every
 * function here is pure over the bitmap and allocates nothing.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: RegexCharClass (regex/charclass.c — defined in regex/charclass.h)
 * ============================================================================
 * A 256-bit byte-membership bitmap with compile-time case folding.
 *
 * STRUCT FIELDS (Mirroring regex/charclass.h):
 * ----------------------------------------------------------------------------
 *   RegexCharClass {
 *     uint8_t bits[REGEX_CLASS_BYTES];  // one bit per byte value
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - RegexCharClass_clear(cls)
 *   - RegexCharClass_addByte(cls, byte, insensitive)
 *   - RegexCharClass_addRange(cls, lo, hi, insensitive)
 *   - RegexCharClass_addPredefined(cls, letter)
 *   - RegexCharClass_isPredefined(letter)
 *   - RegexCharClass_negate(cls)
 *   - RegexCharClass_test(cls, byte)
 *
 * Private Core Functions: (.c static)
 *   - setBit(cls, byte)
 *   - addFoldedByte(cls, byte, insensitive)
 *
 * Admission, ownership, failure:
 *   - Pure bitmap operations: no allocation, no failure path, no lifetime.
 *   - clear leaves the set empty; addRange with lo > hi adds nothing (the
 *     parser rejects an inverted range before calling).
 * ============================================================================
 */

// Sets one byte's bit.
static void setBit(RegexCharClass *cls, uint8_t byte) {
    (*cls).bits[byte >> 3] |= (uint8_t)(1u << (byte & 7u));
}

// Sets a byte and, when folding, its opposite ASCII case (letters only).
static void addFoldedByte(RegexCharClass *cls, uint8_t byte, bool insensitive) {
    setBit(cls, byte);
    if (!insensitive)
        return;
    if (byte >= (uint8_t) 'A' && byte <= (uint8_t) 'Z')
        setBit(cls, (uint8_t)(byte + 32u));
    else if (byte >= (uint8_t) 'a' && byte <= (uint8_t) 'z')
        setBit(cls, (uint8_t)(byte - 32u));
}

void RegexCharClass_clear(RegexCharClass *cls) {
    for (uint32_t i = 0; i < REGEX_CLASS_BYTES; i++)
        (*cls).bits[i] = 0;
}

void RegexCharClass_addByte(RegexCharClass *cls, uint8_t byte, bool insensitive) {
    addFoldedByte(cls, byte, insensitive);
}

void RegexCharClass_addRange(RegexCharClass *cls, uint8_t lo, uint8_t hi, bool insensitive) {
    if (hi < lo)
        return;
    for (uint32_t c = (uint32_t) lo; c <= (uint32_t) hi; c++)
        addFoldedByte(cls, (uint8_t) c, insensitive);
}

bool RegexCharClass_isPredefined(char letter) {
    return letter == 'd' || letter == 'D' || letter == 'w' || letter == 'W' ||
           letter == 's' || letter == 'S';
}

void RegexCharClass_addPredefined(RegexCharClass *cls, char letter) {
    switch (letter) {
    case 'd':
        for (uint32_t c = '0'; c <= '9'; c++)
            setBit(cls, (uint8_t) c);
        break;
    case 'D':
        for (uint32_t c = 0; c < 256u; c++)
            if (c < '0' || c > '9')
                setBit(cls, (uint8_t) c);
        break;
    case 'w':
        for (uint32_t c = '0'; c <= '9'; c++) setBit(cls, (uint8_t) c);
        for (uint32_t c = 'A'; c <= 'Z'; c++) setBit(cls, (uint8_t) c);
        for (uint32_t c = 'a'; c <= 'z'; c++) setBit(cls, (uint8_t) c);
        setBit(cls, (uint8_t) '_');
        break;
    case 'W':
        for (uint32_t c = 0; c < 256u; c++) {
            bool word = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') || c == '_';
            if (!word)
                setBit(cls, (uint8_t) c);
        }
        break;
    case 's':
        setBit(cls, (uint8_t) ' ');
        setBit(cls, (uint8_t) '\t');
        setBit(cls, (uint8_t) '\n');
        setBit(cls, (uint8_t) '\r');
        setBit(cls, (uint8_t) '\f');
        setBit(cls, (uint8_t) '\v');
        break;
    case 'S':
        for (uint32_t c = 0; c < 256u; c++) {
            bool space = c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
                         c == '\f' || c == '\v';
            if (!space)
                setBit(cls, (uint8_t) c);
        }
        break;
    default:
        break;
    }
}

void RegexCharClass_negate(RegexCharClass *cls) {
    for (uint32_t i = 0; i < REGEX_CLASS_BYTES; i++)
        (*cls).bits[i] = (uint8_t) ~(*cls).bits[i];
}

bool RegexCharClass_test(const RegexCharClass *cls, uint8_t byte) {
    return ((*cls).bits[byte >> 3] & (uint8_t)(1u << (byte & 7u))) != 0;
}
