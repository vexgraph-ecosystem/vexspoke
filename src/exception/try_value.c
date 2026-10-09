#include "exception/try_value.h"

#include <stdio.h>
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: TryValue
 * ============================================================================
 * The scalar value-or-error return. TryValue_2 builds the pair; TryValue_ok and
 * TryValue_error are the two ergonomic shorthands a call site reads. It is a
 * by-value POD, so it lives on the stack, is returned in registers, and is never
 * freed. Setters and getters are complete and null-safe per the Symmetric
 * Getter/Setter Completeness Law; both string projections are bounded and
 * truncation-flagged per the toString Law. A caller must read `value` only when
 * `code == TRY_OK`.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: TryValue (exception/try_value.c)
 * ============================================================================
 * the scalar value-or-error return, paired with TryPtr for the pointer form.
 *
 * STRUCT FIELDS (Mirroring exception/try_value.h):
 * ----------------------------------------------------------------------------
 *   TryValue {
 *     uint64_t value; // success payload; undefined unless code == TRY_OK
 *     TryCode  code;  // TRY_OK or the named reason
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - TryValue_2(value, code)
 *   - TryValue_ok(value)
 *   - TryValue_error(code)
 *
 * Setters:
 *   - TryValue_setValue(self, value)
 *   - TryValue_setCode(self, code)
 *
 * Getters:
 *   - TryValue_getValue(self)
 *   - TryValue_getCode(self)
 *   - TryValue_isOk(self)
 *
 * String projections:
 *   - TryValue_toString(self, dest, cap, outTruncated)
 *   - TryValue_toStringStruct(self, dest, cap, outTruncated)
 * ============================================================================
 */

// Constructs a result containing the scalar payload and status code.
TryValue TryValue_2(uint64_t value, TryCode code) {
    TryValue self;
    self.value = value;
    self.code = code;
    return self;
}

// Constructs a successful result carrying value.
TryValue TryValue_ok(uint64_t value) {
    return TryValue_2(value, TRY_OK);
}

// Constructs an error result carrying zero and code.
TryValue TryValue_error(TryCode code) {
    return TryValue_2(0u, code);
}

// Returns the stored scalar, or zero when self is nullptr.
uint64_t TryValue_getValue(const TryValue *self) {
    if (!self)
        return 0u;
    return (*self).value;
}

// Replaces the scalar payload when self is non-null.
void TryValue_setValue(TryValue *self, uint64_t value) {
    if (!self)
        return;
    (*self).value = value;
}

// Returns the stored code, or TRY_NULL_ARG when self is nullptr.
TryCode TryValue_getCode(const TryValue *self) {
    if (!self)
        return TRY_NULL_ARG;
    return (*self).code;
}

// Replaces the result code when self is non-null.
void TryValue_setCode(TryValue *self, TryCode code) {
    if (!self)
        return;
    (*self).code = code;
}

// Reports whether a non-null result carries TRY_OK.
bool TryValue_isOk(const TryValue *self) {
    if (!self)
        return false;
    return (*self).code == TRY_OK;
}

// Formats a value-oriented summary into dest and reports capacity truncation.
void TryValue_toString(const TryValue *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!dest || cap == 0u)
        return;
    if (!self) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written;
    if ((*self).code == TRY_OK) {
        written = snprintf(dest, cap, "TryValue(value=%llu, code=TRY_OK)",
                           (unsigned long long) (*self).value);
    } else {
        written = snprintf(dest, cap, "TryValue(code=%s: %s)",
                           TryCode_name((*self).code),
                           TryCode_message((*self).code));
    }
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}

// Formats the result fields into dest and reports capacity truncation.
void TryValue_toStringStruct(const TryValue *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!dest || cap == 0u)
        return;
    if (!self) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "TryValue { value=%llu, code=%s }",
                           (unsigned long long) (*self).value,
                           TryCode_name((*self).code));
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}
