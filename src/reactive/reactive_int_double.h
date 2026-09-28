#ifndef REACTIVE_REACTIVE_INT_DOUBLE_H
#define REACTIVE_REACTIVE_INT_DOUBLE_H

#include "primitive/int_double.h"
#include "reactive/generic.h"

// reactive/reactive_int_double.h — an observable IntDouble, STAMPED by TYPEDEF_REACTIVE.
//
// 16 bytes — too big for one engine word — so the word holds an IntDouble* (a
// published pair); a rebind is a change. Spell it Reactive(IntDouble) at call sites.

TYPEDEF_REACTIVE(IntDouble);

#endif
