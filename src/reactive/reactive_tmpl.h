// reactive/reactive_tmpl.h — the typed-reactive DECLARATION template.
//
// TEMPLATE, NOT A HEADER: no include guard by design — it is included once per
// instantiation (each instantiation's own header carries the guard). The caller
// defines VEX_NAME (the class name) and VEX_T (the value type) before the include
// and undefs them after. The caller also defines its own arity chooser
// (`#define Cls(...)`): a template cannot emit a `#define` whose name is a macro
// (the preprocessor does not expand a macro on the left of a `#define`).
//
//   #define VEX_NAME ReactiveInt
//   #define VEX_T    int32_t
//   #include "reactive/reactive_tmpl.h"
//   #undef VEX_NAME
//   #undef VEX_T
//   #define ReactiveInt(...) CONSTRUCTOR_DISPATCH(ReactiveInt, __VA_ARGS__)
//
// The class embeds the engine FIRST, so a typed reactive pointer IS a Reactive
// pointer (embed-first, the Reactive engine contract).

typedef struct VEX_NAME {
    Reactive base;
} VEX_NAME;

VEX_NAME *VEX_CAT(VEX_NAME, _0)(void);
VEX_NAME *VEX_CAT(VEX_NAME, _1)(VEX_T initial);
void VEX_CAT(VEX_NAME, _free)(VEX_NAME *self);
void VEX_CAT(VEX_NAME, _set)(VEX_NAME *self, VEX_T value);
VEX_T VEX_CAT(VEX_NAME, _get)(const VEX_NAME *self);
