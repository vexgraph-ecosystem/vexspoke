#ifndef REACTIVE_REACTIVE_VEC2_H
#define REACTIVE_REACTIVE_VEC2_H

#include "lang/vec2/vec2.h"
#include "reactive/generic.h"

// reactive/reactive_vec2.h — an observable 2D vector, STAMPED by TYPEDEF_REACTIVE.
//
// The word holds a Vec2* (a published vector); a rebind is a change. Spell it
// Reactive(Vec2) at call sites.

TYPEDEF_REACTIVE(Vec2);

#endif
