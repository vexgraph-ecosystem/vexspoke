#ifndef REACTIVE_REACTIVE_LONG_FLOAT_H
#define REACTIVE_REACTIVE_LONG_FLOAT_H

#include "primitive/long_float.h"
#include "reactive/generic.h"

// reactive/reactive_long_float.h — an observable LongFloat, STAMPED by TYPEDEF_REACTIVE.
//
// 16 bytes — too big for one engine word — so the word holds a LongFloat* (a
// published pair); a rebind is a change. Spell it Reactive(LongFloat) at call sites.

TYPEDEF_REACTIVE(LongFloat);

#endif
