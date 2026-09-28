#ifndef REACTIVE_REACTIVE_VEC4_H
#define REACTIVE_REACTIVE_VEC4_H

#include "lang/vec4/vec4.h"
#include "reactive/generic.h"

// reactive/reactive_vec4.h — an observable 4D vector, STAMPED by TYPEDEF_REACTIVE.
//
// The word holds a Vec4* (a published vector); a rebind is a change. Spell it
// Reactive(Vec4) at call sites.

TYPEDEF_REACTIVE(Vec4);

#endif
