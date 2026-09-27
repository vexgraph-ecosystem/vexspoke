#include "exception/try_ptr.h"

#include <stdio.h>
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: TryPtr
 * ============================================================================
 * The pointer value-or-error return, sibling to TryValue. TryPtr_2 builds the
 * pair; TryPtr_ok and TryPtr_error are the two shorthands. A by-value POD, it
 * lives on the stack, returns in registers, and is never freed. It is used where
 * nullptr cannot carry the answer on its own — a nullable field, or a failure
 * whose reason must reach the caller. Setters/getters are complete and null-safe
 * per the Symmetric Getter/Setter Completeness Law; both string projections are
 * bounded and truncation-flagged per the toString Law.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: TryPtr (exception/try_ptr.c)
 * LEVEL: L2 — Behavior (value-or-error primitive)
 * ============================================================================
 * the pointer value-or-error return, paired with TryValue for the scalar form.
 *
 * STRUCT FIELDS (Mirroring exception/try_ptr.h):
 * ----------------------------------------------------------------------------
 *   TryPtr {
 *     void   *value; // success payload; undefined unless code == TRY_OK
 *     TryCode code;  // TRY_OK or the named reason
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - TryPtr_2(value, code)
 *   - TryPtr_ok(value)
 *   - TryPtr_error(code)
 *
 * Setters:
 *   - TryPtr_setValue(self, value)
 *   - TryPtr_setCode(self, code)
 *
 * Getters:
 *   - TryPtr_getValue(self)
 *   - TryPtr_getCode(self)
 *   - TryPtr_isOk(self)
 *
 * String projections:
 *   - TryPtr_toString(self, dest, cap, outTruncated)
 *   - TryPtr_toStringStruct(self, dest, cap, outTruncated)
 * ============================================================================
 */

TryPtr TryPtr_2(void *value, TryCode code) {
    TryPtr self;
    self.value = value;
    self.code = code;
    return self;
}

TryPtr TryPtr_ok(void *value) {
    return TryPtr_2(value, TRY_OK);
}

TryPtr TryPtr_error(TryCode code) {
    return TryPtr_2(nullptr, code);
}

void *TryPtr_getValue(const TryPtr *self) {
    if (!self)
        return nullptr;
    return (*self).value;
}

void TryPtr_setValue(TryPtr *self, void *value) {
    if (!self)
        return;
    (*self).value = value;
}

TryCode TryPtr_getCode(const TryPtr *self) {
    if (!self)
        return TRY_NULL_ARG;
    return (*self).code;
}

void TryPtr_setCode(TryPtr *self, TryCode code) {
    if (!self)
        return;
    (*self).code = code;
}

bool TryPtr_isOk(const TryPtr *self) {
    if (!self)
        return false;
    return (*self).code == TRY_OK;
}

void TryPtr_toString(const TryPtr *self, char *dest, size_t cap, bool *outTruncated) {
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
        written = snprintf(dest, cap, "TryPtr(value=%p, code=TRY_OK)", (*self).value);
    } else {
        written = snprintf(dest, cap, "TryPtr(code=%s: %s)",
                           TryCode_name((*self).code),
                           TryCode_message((*self).code));
    }
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}

void TryPtr_toStringStruct(const TryPtr *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (!dest || cap == 0u)
        return;
    if (!self) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "TryPtr { value=%p, code=%s }",
                           (*self).value,
                           TryCode_name((*self).code));
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}
