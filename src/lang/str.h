#ifndef LANG_STR_H
#define LANG_STR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// lang/str.h — the bounded string builder (the toString Law's engine).
//
// Str is the ONE place a bound is checked: every write is capped at `cap` and
// flags `truncated` on a cut (the Cold-Strict, Hot-Minimal Validation Law's
// Truncation-Never-Silent clause). Every class's toString / toStringStruct
// builds through one Str and reports Str_isTruncated, so no class hand-rolls a
// bounds check. Always null-terminated (a byte is reserved for the terminator).

typedef struct Str {
    char  *dest;       // borrowed destination buffer
    size_t cap;        // total capacity (including the terminator)
    size_t len;        // bytes written so far (excluding the terminator)
    bool   truncated;  // a write was cut
} Str;

// --- Constructors ---
// Bind a destination. cap 0 (or a null dest) makes every write a no-op that
// flags truncated.
void Str_init(Str *s, char *dest, size_t cap);

// --- Core functions (all bounded) ---
void Str_put(Str *s, const char *text);       // append a C string
void Str_putc(Str *s, char c);                // append one char
void Str_putQuoted(Str *s, const char *text); // append "text" with \n/\t/\" escapes
void Str_printf(Str *s, const char *fmt, ...); // bounded printf (cold path)

// --- Getters (the Symmetric Getter/Setter Completeness Law: null-safe) ---
bool   Str_isTruncated(const Str *s);
size_t Str_length(const Str *s);
const char *Str_cstr(const Str *s);           // the written buffer (null-safe)

#endif // LANG_STR_H
