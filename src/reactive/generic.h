#ifndef REACTIVE_GENERIC_H
#define REACTIVE_GENERIC_H

#include "reactive/reactive.h"

// reactive/generic.h — the reactive generics surface.
//
// Reactive(T) is the ONE spelling of a typed reactive. It pastes the type name
// straight onto `Reactive`, so Reactive(int) -> ReactiveInt, Reactive(Buffer) ->
// ReactiveBuffer, Reactive(Vec4) -> ReactiveVec4 — the concrete, single-class-per-
// file class name, with no translation table.
//
// The type argument is always a LEAF type (int, Vec4, Buffer, String, ...).
// Nesting is rejected by intent: a reactive over a probable is the MERGED
// ReactiveProbable(T), never Reactive(Probable(T)).
//
// VEX_CAT is the two-level paste: a `##` operand is never expanded, so the outer
// macro — which is NOT a paste context — expands its argument first, then the
// inner paste joins the results (GCC Argument Prescan; ISO C23 6.10.3.1).
//
// OBJECT REACTIVES — the one-liner. Because everything is a pointer, a reactive
// over an object type stores a `NAME *` with uniform pass-through packing, so one
// macro covers every object type:
//
//     // reactive_buffer.h
//     TYPEDEF_REACTIVE(Buffer);      // stamps the ReactiveBuffer class + API
//
//     // reactive_buffer.c
//     IMPLEMENT_REACTIVE(Buffer);    // stamps the bodies
//
// One caveat the preprocessor cannot erase: a macro cannot define a macro, so no
// macro here can emit the per-class arity chooser. Construction is explicit —
// ReactiveBuffer_1(value) — or the header writes `#define ReactiveBuffer(...)`
// by hand. `NAME` must be a single token (Buffer, Cell, ...), never `unsigned char`.

#define VEX_CAT_(a, b) a##b
#define VEX_CAT(a, b)  VEX_CAT_(a, b)

// The family constructors. Reactive(Int) -> ReactiveInt; Reactive(Buffer) -> ReactiveBuffer.
#define Reactive(T)         VEX_CAT(Reactive, T)
#define ReactiveProbable(T) VEX_CAT(ReactiveProbable, T)

// Internal paste helpers for the object one-liner.
#define VEX_RCLASS(NAME)   VEX_CAT(Reactive, NAME)
#define VEX_RFN(NAME, SUF) VEX_CAT(VEX_CAT(Reactive, NAME), SUF)

// Declarations: the class struct + the public API. Requires a trailing `;` at
// the call site (like any struct declaration).
#define TYPEDEF_REACTIVE(NAME)                                          \
    typedef struct VEX_RCLASS(NAME) {                                   \
        Reactive base;                                                  \
    } VEX_RCLASS(NAME);                                                 \
    VEX_RCLASS(NAME) *VEX_RFN(NAME, _0)(void);                          \
    VEX_RCLASS(NAME) *VEX_RFN(NAME, _1)(NAME *initial);                 \
    void VEX_RFN(NAME, _free)(VEX_RCLASS(NAME) *self);                  \
    void VEX_RFN(NAME, _set)(VEX_RCLASS(NAME) *self, NAME *value);      \
    NAME *VEX_RFN(NAME, _get)(const VEX_RCLASS(NAME) *self)

// Definitions: the same API, bodies included. The source file must have already
// included nio/mem.h and oop/type.h (Memory_alloc/Memory_free, TYPE_REACTIVE).
#define IMPLEMENT_REACTIVE(NAME)                                        \
    VEX_RCLASS(NAME) *VEX_RFN(NAME, _1)(NAME *initial) {                \
        VEX_RCLASS(NAME) *self = (VEX_RCLASS(NAME)*)                    \
            Memory_alloc(TYPE_REACTIVE, sizeof(VEX_RCLASS(NAME)));      \
        if (self == nullptr)                                            \
            return nullptr;                                             \
        Reactive_init(&(*self).base, (uintptr_t) initial);              \
        return self;                                                    \
    }                                                                   \
    VEX_RCLASS(NAME) *VEX_RFN(NAME, _0)(void) {                         \
        return VEX_RFN(NAME, _1)(nullptr);                              \
    }                                                                   \
    void VEX_RFN(NAME, _free)(VEX_RCLASS(NAME) *self) {                 \
        if (self == nullptr)                                            \
            return;                                                     \
        Reactive_shutdown(&(*self).base);                               \
        Memory_free(self);                                              \
    }                                                                   \
    void VEX_RFN(NAME, _set)(VEX_RCLASS(NAME) *self, NAME *value) {     \
        if (self == nullptr)                                            \
            return;                                                     \
        Reactive_set(&(*self).base, (uintptr_t) value);                 \
    }                                                                   \
    NAME *VEX_RFN(NAME, _get)(const VEX_RCLASS(NAME) *self) {           \
        if (self == nullptr)                                            \
            return nullptr;                                             \
        return (NAME*) Reactive_get((Reactive*) &(*self).base);         \
    }

#endif
