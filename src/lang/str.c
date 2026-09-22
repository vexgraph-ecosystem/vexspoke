#include "lang/str.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/getter.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Str
 * ============================================================================
 * The bounded string builder — the engine the toString Law stands on. Every
 * write is capped at `cap` and flags `truncated` on a cut, so a class never
 * hand-rolls a bounds check and a deep/large dump can never overflow. The buffer
 * is always null-terminated (one byte reserved).
 *
 * Str owns nothing: it borrows the caller's buffer. It is the Cold-Strict,
 * Hot-Minimal Validation Law's Truncation-Never-Silent clause made concrete —
 * a cut is a loud flag, never a silent overwrite.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Str (lang/str.c)
 * LEVEL: L1 — File Metadata (a bounded string builder value)
 * ============================================================================
 * SUMMARY:
 *   Borrows a destination buffer; every append is bounded and flags truncation.
 *
 * STRUCT FIELDS (Mirroring lang/str.h):
 * ----------------------------------------------------------------------------
 *   char *dest;       // borrowed destination buffer
 *   size_t cap;       // total capacity (including the terminator)
 *   size_t len;       // bytes written so far (excluding the terminator)
 *   bool truncated;   // a write was cut
 *
 * PRIVATE HELPERS:
 * ----------------------------------------------------------------------------
 *   (none)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - Str_init(s, dest, cap)
 *
 * Public Core Functions: (.h)
 *   - Str_put(s, text) / Str_putc(s, c) / Str_putQuoted(s, text) / Str_printf(s, fmt, ...)
 *
 * Public Getters: (.h)
 *   - Str_isTruncated(s) / Str_length(s) / Str_cstr(s)
 * ============================================================================
 */

// Remaining writable bytes (excluding the reserved terminator).
static size_t room(const Str *s) {
    if (s == nullptr || (*s).dest == nullptr || (*s).cap == 0)
        return 0;
    return ((*s).len < (*s).cap - 1u) ? ((*s).cap - 1u - (*s).len) : 0u;
}

void Str_init(Str *s, char *dest, size_t cap) {
    if (s == nullptr)
        return;
    (*s).dest = dest;
    (*s).cap = cap;
    (*s).len = 0;
    (*s).truncated = false;
    if (dest != nullptr && cap > 0)
        dest[0] = '\0';
}

void Str_putc(Str *s, char c) {
    if (s == nullptr)
        return;
    if ((*s).dest == nullptr || (*s).cap == 0) {
        (*s).truncated = true;
        return;
    }
    if ((*s).len + 1u >= (*s).cap) {
        (*s).truncated = true;
        return;
    }
    (*s).dest[(*s).len++] = c;
    (*s).dest[(*s).len] = '\0';
}

void Str_put(Str *s, const char *text) {
    if (s == nullptr || text == nullptr)
        return;
    size_t n = strlen(text);
    size_t r = room(s);
    if (n > r) {
        n = r;
        (*s).truncated = true;
    }
    if (n > 0) {
        memcpy((*s).dest + (*s).len, text, n);
        (*s).len += n;
        (*s).dest[(*s).len] = '\0';
    }
}

void Str_putQuoted(Str *s, const char *text) {
    if (s == nullptr)
        return;
    Str_putc(s, '"');
    if (text != nullptr) {
        for (const char *p = text; *p != '\0'; p++) {
            switch (*p) {
                case '\n': Str_put(s, "\\n"); break;
                case '\t': Str_put(s, "\\t"); break;
                case '\r': Str_put(s, "\\r"); break;
                case '"':  Str_put(s, "\\\""); break;
                case '\\': Str_put(s, "\\\\"); break;
                default:   Str_putc(s, *p); break;
            }
        }
    }
    Str_putc(s, '"');
}

void Str_printf(Str *s, const char *fmt, ...) {
    if (s == nullptr || fmt == nullptr)
        return;
    if ((*s).dest == nullptr || (*s).cap == 0) {
        (*s).truncated = true;
        return;
    }
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf((*s).dest + (*s).len, (*s).cap - (*s).len, fmt, args);
    va_end(args);
    if (written < 0) {
        (*s).truncated = true;
        return;
    }
    size_t w = (size_t) written;
    size_t r = room(s);
    if (w > r) {
        (*s).len = (*s).cap - 1u;   // vsnprintf wrote r bytes + terminator
        (*s).truncated = true;
    } else {
        (*s).len += w;
    }
}

;;GETTER
bool Str_isTruncated(const Str *s) {
    return s ? (*s).truncated : false;
}

;;GETTER
size_t Str_length(const Str *s) {
    return s ? (*s).len : 0u;
}

;;GETTER
const char *Str_cstr(const Str *s) {
    return (s && (*s).dest) ? (*s).dest : "";
}
