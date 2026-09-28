#ifndef REACTIVE_REACTIVE_LONG_DOUBLE_H
#define REACTIVE_REACTIVE_LONG_DOUBLE_H

#include "primitive/long_double.h"
#include "reactive/generic.h"

// reactive/reactive_long_double.h — an observable LongDouble, STAMPED by TYPEDEF_REACTIVE.
//
// 16 bytes — too big for one engine word — so the word holds a LongDouble* (a
// published pair); a rebind is a change. Spell it Reactive(LongDouble) at call sites.

TYPEDEF_REACTIVE(LongDouble);

#endif
