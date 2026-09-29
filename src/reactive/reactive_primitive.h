#ifndef REACTIVE_REACTIVE_PRIMITIVE_H
#define REACTIVE_REACTIVE_PRIMITIVE_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "primitive/int_float.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_primitive.h — the WHOLE scalar reactive family in one file.
//
// ;;INTENTION("the scalar reactives are ten instantiations of one template that
// differ only in VEX_T — mechanical stamps, not hand-authored classes. The
// Single Class Per File Law is waived BY INTENT here: one file pair is clearer
// than ten identical ones. Per the Conflict Triage Law.")
//
// Reactive(int)/Reactive(float)/... resolve here (via the lowercase bridge in
// generic.h). Each class is identical but for its value type and packing:
// integer cast, float/double bit-cast, the IntFloat pair, or a pointer word.

#define VEX_NAME ReactiveBool
#define VEX_T    bool
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveBool(...) CONSTRUCTOR_DISPATCH(ReactiveBool, __VA_ARGS__)

#define VEX_NAME ReactiveByte
#define VEX_T    int8_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveByte(...) CONSTRUCTOR_DISPATCH(ReactiveByte, __VA_ARGS__)

#define VEX_NAME ReactiveShort
#define VEX_T    int16_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveShort(...) CONSTRUCTOR_DISPATCH(ReactiveShort, __VA_ARGS__)

#define VEX_NAME ReactiveChar
#define VEX_T    uint16_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveChar(...) CONSTRUCTOR_DISPATCH(ReactiveChar, __VA_ARGS__)

#define VEX_NAME ReactiveInt
#define VEX_T    int32_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveInt(...) CONSTRUCTOR_DISPATCH(ReactiveInt, __VA_ARGS__)

#define VEX_NAME ReactiveLong
#define VEX_T    int64_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveLong(...) CONSTRUCTOR_DISPATCH(ReactiveLong, __VA_ARGS__)

#define VEX_NAME ReactiveFloat
#define VEX_T    float
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveFloat(...) CONSTRUCTOR_DISPATCH(ReactiveFloat, __VA_ARGS__)

#define VEX_NAME ReactiveDouble
#define VEX_T    double
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveDouble(...) CONSTRUCTOR_DISPATCH(ReactiveDouble, __VA_ARGS__)

#define VEX_NAME ReactiveString
#define VEX_T    const uint8_t *
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveString(...) CONSTRUCTOR_DISPATCH(ReactiveString, __VA_ARGS__)

#define VEX_NAME ReactiveIntFloat
#define VEX_T    IntFloat
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T
#define ReactiveIntFloat(...) CONSTRUCTOR_DISPATCH(ReactiveIntFloat, __VA_ARGS__)

#endif
