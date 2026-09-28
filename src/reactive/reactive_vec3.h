#ifndef REACTIVE_REACTIVE_VEC3_H
#define REACTIVE_REACTIVE_VEC3_H

#include "lang/vec3/vec3.h"
#include "reactive/generic.h"

// reactive/reactive_vec3.h — an observable 3D vector, STAMPED by TYPEDEF_REACTIVE.
//
// The word holds a Vec3* (a published vector); a rebind is a change. Spell it
// Reactive(Vec3) at call sites.

TYPEDEF_REACTIVE(Vec3);

#endif
