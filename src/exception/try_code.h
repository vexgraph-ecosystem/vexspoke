#ifndef EXCEPTION_TRY_CODE_H
#define EXCEPTION_TRY_CODE_H

#include <stdbool.h>

// exception/try_code.h — the TryCode taxonomy: the named reason a Try carries.
//
// A Try returns a value OR a code. The code is the machine-readable half; the
// prose half comes from TryCode_name / TryCode_message — static literals, so no
// Try ever allocates (the Cold-Strict, Hot-Minimal Validation Law). Codes are
// named, never bare numbers (the No Hardcoding Law).

typedef enum TryCode {
    TRY_OK = 0,       // success: the Try's value is valid
    TRY_NULL_ARG,     // a required pointer argument was nullptr
    TRY_BOUNDS,       // index outside the collection's active range
    TRY_OVERFLOW,     // size or stride arithmetic would wrap
    TRY_EMPTY,        // the source held no element
    TRY_NOT_FOUND,    // the named thing is absent
    TRY_IO,           // a file or stream read/write failed
    TRY_FORMAT,       // bytes were present but malformed
    TRY_UNSUPPORTED   // the operation is not supported here
} TryCode;

// True when code == TRY_OK — the single success test (the Hot-Path Minimal
// Guard Law: one compare, no table lookup). Null-safe is meaningless here; the
// code is a value.
bool TryCode_isOk(TryCode code);

// The short symbolic name, e.g. "TRY_BOUNDS". Never nullptr.
const char *TryCode_name(TryCode code);

// The human sentence, e.g. "index outside the active range". Never nullptr.
const char *TryCode_message(TryCode code);

#endif
