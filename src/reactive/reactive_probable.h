#ifndef REACTIVE_REACTIVE_PROBABLE_H
#define REACTIVE_REACTIVE_PROBABLE_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_probable.h — the ReactiveProbable family in one file.
//
// ;;INTENTION("the probable reactives are instantiations of one template that
// differ only in VEX_T (as the scalar family is). Mechanical stamps, not
// hand-authored classes — one file pair. Per the Conflict Triage Law.")
//
// The merged observable probable: { value, chance } with chance in [0, 1]; get()
// yields the value with that probability, else the empty value. There is no
// nesting (Reactive(Probable(T)) does not exist) — the reactivity lives inside.

#define VEX_NAME ReactiveProbableInt
#define VEX_T    int32_t
#include "reactive/reactive_probable_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveProbableInt(...) CONSTRUCTOR_DISPATCH(ReactiveProbableInt, __VA_ARGS__)

#endif
