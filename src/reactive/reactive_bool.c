// reactive/reactive_bool.c — the Java-boolean typed reactive.

#include "reactive/reactive_bool.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveBool
 * ============================================================================
 * A word-sized typed reactive: it embeds the engine (reactive/reactive.h) and
 * the engine's atomic word IS the boolean (0 or 1). One compare detects a
 * change; the engine supplies the observers, dirty flag, and owner-affine drain.
 * Java semantics: boolean. Arena-allocated (TYPE_REACTIVE_BOOL) or embedded.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveBool (reactive/reactive_bool.c)
 * LEVEL: L2 — Behavior (typed reactive)
 * ============================================================================
 * the boolean typed reactive (an embedded Reactive engine).
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_bool.h):
 * ----------------------------------------------------------------------------
 *   ReactiveBool { Reactive base; } // embed-first; word = 0/1
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveBool_0(), ReactiveBool_1(initial), _free
 * Public Setters: (.h) ReactiveBool_set(self, value)
 * Public Getters: (.h) ReactiveBool_get(self)
 * ============================================================================
 */

ReactiveBool *ReactiveBool_1(bool initial) {
    ReactiveBool *self = (ReactiveBool*) Memory_alloc(TYPE_REACTIVE_BOOL, sizeof(ReactiveBool));
    if (self == nullptr)
        return nullptr;
    Reactive_init(&(*self).base, initial ? 1u : 0u);
    return self;
}

ReactiveBool *ReactiveBool_0(void) {
    return ReactiveBool_1(false);
}

void ReactiveBool_free(ReactiveBool *self) {
    if (self == nullptr)
        return;
    Reactive_shutdown(&(*self).base);
    Memory_free(self);
}

void ReactiveBool_set(ReactiveBool *self, bool value) {
    if (self == nullptr)
        return;
    Reactive_set(&(*self).base, value ? 1u : 0u);
}

bool ReactiveBool_get(const ReactiveBool *self) {
    if (self == nullptr)
        return false;
    return Reactive_get((Reactive*) &(*self).base) != 0u;
}
