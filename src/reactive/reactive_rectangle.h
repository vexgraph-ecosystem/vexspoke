#ifndef REACTIVE_REACTIVE_RECTANGLE_H
#define REACTIVE_REACTIVE_RECTANGLE_H

#include "lang/rect/rectangle.h"
#include "reactive/generic.h"

// reactive/reactive_rectangle.h — an observable Rectangle, STAMPED by TYPEDEF_REACTIVE.
//
// The word holds a Rectangle* (a published rect); a rebind is a change. Spell it
// Reactive(Rectangle) at call sites.

TYPEDEF_REACTIVE(Rectangle);

#endif
