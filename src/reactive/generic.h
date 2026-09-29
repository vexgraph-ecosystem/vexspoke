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
// VEX_CAT is the two-level paste: a `##` operand is never expanded, so the outer
// macro — which is NOT a paste context — expands its argument first, then the
// inner paste joins the results (GCC Argument Prescan; ISO C23 6.10.3.1).
//
// Two ways to make a typed reactive:
//
//   1. Scalars and the family use reactive/reactive_tmpl.{h,inc} (per-type hooks).
//   2. Any OBJECT type is one line each (uniform pass-through, value = NAME*):
//
//          // reactive_buffer.h
//          TYPEDEF_REACTIVE(Buffer);      // stamps the class + API + channels
//          // reactive_buffer.c
//          IMPLEMENT_REACTIVE(Buffer);    // stamps the bodies
//
// A macro cannot define a macro, so neither form can emit the per-class arity
// chooser: construction is explicit (ReactiveBuffer_1(value)) or the header
// writes `#define ReactiveBuffer(...)` by hand.

#define VEX_CAT_(a, b) a##b
#define VEX_CAT(a, b)  VEX_CAT_(a, b)

// The family constructors. Reactive(Int) -> ReactiveInt; Reactive(Buffer) -> ReactiveBuffer.
#define Reactive(T)         VEX_CAT(Reactive, T)
#define ReactiveProbable(T) VEX_CAT(ReactiveProbable, T)

// --- The typed channel surface (shared by the template and TYPEDEF_REACTIVE) ---
//
// The callback receives the VALUE — never the reactive — because the value IS the
// payload. remove matches the function-pointer address (there is no userdata).
// DECL goes in a header; DEF goes in a source that has already included the
// header (so the channel typedefs are visible). FROMW unpacks the engine word.

#define VEX_CHANNELS_DECL(NAME, T)                                          \
    typedef void (*VEX_CAT(NAME, SetFn))(T value);                          \
    typedef void (*VEX_CAT(NAME, ChangedFn))(T value);                      \
    typedef void (*VEX_CAT(NAME, GetFn))(T value);                          \
    typedef void (*VEX_CAT(NAME, NullptrFn))(void);                         \
    bool VEX_CAT(NAME, _addOnSet)(NAME *self, VEX_CAT(NAME, SetFn) fn);     \
    bool VEX_CAT(NAME, _removeOnSet)(NAME *self, VEX_CAT(NAME, SetFn) fn);  \
    bool VEX_CAT(NAME, _addOnChanged)(NAME *self, VEX_CAT(NAME, ChangedFn) fn); \
    bool VEX_CAT(NAME, _removeOnChanged)(NAME *self, VEX_CAT(NAME, ChangedFn) fn); \
    bool VEX_CAT(NAME, _addOnGet)(NAME *self, VEX_CAT(NAME, GetFn) fn);     \
    bool VEX_CAT(NAME, _removeOnGet)(NAME *self, VEX_CAT(NAME, GetFn) fn);  \
    bool VEX_CAT(NAME, _addOnNullptr)(NAME *self, VEX_CAT(NAME, NullptrFn) fn); \
    bool VEX_CAT(NAME, _removeOnNullptr)(NAME *self, VEX_CAT(NAME, NullptrFn) fn)

#define VEX_CHANNELS_DEF(NAME, FROMW)                                       \
    static void VEX_CAT(NAME, _setTramp)(Reactive *r, uintptr_t v, void *ud) { \
        (void) r; ((VEX_CAT(NAME, SetFn)) ud)(FROMW(v));                    \
    }                                                                       \
    static void VEX_CAT(NAME, _changedTramp)(Reactive *r, uintptr_t o, uintptr_t n, void *ud) { \
        (void) r; (void) o; ((VEX_CAT(NAME, ChangedFn)) ud)(FROMW(n));      \
    }                                                                       \
    static void VEX_CAT(NAME, _getTramp)(Reactive *r, uintptr_t v, void *ud) { \
        (void) r; ((VEX_CAT(NAME, GetFn)) ud)(FROMW(v));                    \
    }                                                                       \
    static void VEX_CAT(NAME, _nullptrTramp)(Reactive *r, void *ud) {       \
        (void) r; ((VEX_CAT(NAME, NullptrFn)) ud)();                        \
    }                                                                       \
    bool VEX_CAT(NAME, _addOnSet)(NAME *self, VEX_CAT(NAME, SetFn) fn) {    \
        if (self == nullptr) return false;                                  \
        return Reactive_watchSet(&(*self).base, VEX_CAT(NAME, _setTramp), (void*) fn); \
    }                                                                       \
    bool VEX_CAT(NAME, _removeOnSet)(NAME *self, VEX_CAT(NAME, SetFn) fn) { \
        if (self == nullptr) return false;                                  \
        return Reactive_unwatchSet(&(*self).base, VEX_CAT(NAME, _setTramp), (void*) fn); \
    }                                                                       \
    bool VEX_CAT(NAME, _addOnChanged)(NAME *self, VEX_CAT(NAME, ChangedFn) fn) { \
        if (self == nullptr) return false;                                  \
        return Reactive_watchChanged(&(*self).base, VEX_CAT(NAME, _changedTramp), (void*) fn); \
    }                                                                       \
    bool VEX_CAT(NAME, _removeOnChanged)(NAME *self, VEX_CAT(NAME, ChangedFn) fn) { \
        if (self == nullptr) return false;                                  \
        return Reactive_unwatchChanged(&(*self).base, VEX_CAT(NAME, _changedTramp), (void*) fn); \
    }                                                                       \
    bool VEX_CAT(NAME, _addOnGet)(NAME *self, VEX_CAT(NAME, GetFn) fn) {    \
        if (self == nullptr) return false;                                  \
        return Reactive_watchGet(&(*self).base, VEX_CAT(NAME, _getTramp), (void*) fn); \
    }                                                                       \
    bool VEX_CAT(NAME, _removeOnGet)(NAME *self, VEX_CAT(NAME, GetFn) fn) { \
        if (self == nullptr) return false;                                  \
        return Reactive_unwatchGet(&(*self).base, VEX_CAT(NAME, _getTramp), (void*) fn); \
    }                                                                       \
    bool VEX_CAT(NAME, _addOnNullptr)(NAME *self, VEX_CAT(NAME, NullptrFn) fn) { \
        if (self == nullptr) return false;                                  \
        return Reactive_watchNullptr(&(*self).base, VEX_CAT(NAME, _nullptrTramp), (void*) fn); \
    }                                                                       \
    bool VEX_CAT(NAME, _removeOnNullptr)(NAME *self, VEX_CAT(NAME, NullptrFn) fn) { \
        if (self == nullptr) return false;                                  \
        return Reactive_unwatchNullptr(&(*self).base, VEX_CAT(NAME, _nullptrTramp), (void*) fn); \
    }

// --- The object one-liner --------------------------------------------------

#define VEX_RCLASS(NAME)   VEX_CAT(Reactive, NAME)
#define VEX_RFN(NAME, SUF) VEX_CAT(VEX_CAT(Reactive, NAME), SUF)
#define VEX_OBJECT_FROM(w) ((void*) (w))   // a NAME* round-trips through the word

// Declarations: the class struct + the API + the channels. Requires a trailing
// `;` at the call site (like any struct declaration).
#define TYPEDEF_REACTIVE(NAME)                                          \
    typedef struct VEX_RCLASS(NAME) {                                   \
        Reactive base;                                                  \
    } VEX_RCLASS(NAME);                                                 \
    VEX_RCLASS(NAME) *VEX_RFN(NAME, _0)(void);                          \
    VEX_RCLASS(NAME) *VEX_RFN(NAME, _1)(NAME *initial);                 \
    void VEX_RFN(NAME, _free)(VEX_RCLASS(NAME) *self);                  \
    void VEX_RFN(NAME, _set)(VEX_RCLASS(NAME) *self, NAME *value);      \
    NAME *VEX_RFN(NAME, _get)(const VEX_RCLASS(NAME) *self);            \
    VEX_CHANNELS_DECL(VEX_RCLASS(NAME), NAME *)

// Definitions. The source must have already included nio/mem.h and oop/type.h
// (Memory_alloc/Memory_free, TYPE_REACTIVE).
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
    }                                                                   \
    VEX_CHANNELS_DEF(VEX_RCLASS(NAME), VEX_OBJECT_FROM)

#endif
