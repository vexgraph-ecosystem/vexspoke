#ifndef EXCEPTION_TRY_VALUE_H
#define EXCEPTION_TRY_VALUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "c23/constructor.h"
#include "exception/try_code.h"
#include "annotation/draft.h"

;;DRAFT
// exception/try_value.h — the scalar value-or-error return.
//
// TryValue is a by-value result: { value, code }. It exists because a getter
// whose domain is full — int, uint64, bool, any scalar — has no spare codepoint
// left for a sentinel; a Try is the only way to distinguish "value is 0" from
// "index out of bounds" without an out-parameter. It returns in registers, never
// allocates, and is copied freely up the call chain (the Vertical Integration
// Law: a leaf forwards the Try, the host catches it at its edge).
//
// CONTRACT: `value` is valid ONLY when `code == TRY_OK`.

typedef struct TryValue {
    uint64_t value;   // success payload; undefined unless code == TRY_OK
    TryCode  code;    // TRY_OK or the named reason
} TryValue;

// --- Constructors (by value: no allocation, no pointer) ---
TryValue TryValue_2(uint64_t value, TryCode code);
TryValue TryValue_ok(uint64_t value);
TryValue TryValue_error(TryCode code);
#define TryValue(...) CONSTRUCTOR_DISPATCH(TryValue, __VA_ARGS__)

// --- Setters / Getters (the Symmetric Getter/Setter Completeness Law) ---
uint64_t TryValue_getValue(const TryValue *self);
void     TryValue_setValue(TryValue *self, uint64_t value);
TryCode  TryValue_getCode(const TryValue *self);
void     TryValue_setCode(TryValue *self, TryCode code);
bool     TryValue_isOk(const TryValue *self);

// --- String projections (the toString Law) ---
void TryValue_toString(const TryValue *self, char *dest, size_t cap, bool *outTruncated);
void TryValue_toStringStruct(const TryValue *self, char *dest, size_t cap, bool *outTruncated);

#endif
