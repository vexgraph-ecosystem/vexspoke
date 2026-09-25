#ifndef OBJECTS_REACTIVE_H
#define OBJECTS_REACTIVE_H

// objects/reactive.h — thin re-export shim.
//
// The reactive engine moved to reactive/reactive.h (the reactive/ subsystem:
// one engine + the typed reactives). This shim keeps existing includes
// (graphvex, main, tests) compiling; new code includes "reactive/reactive.h".

#include "reactive/reactive.h"

#endif
