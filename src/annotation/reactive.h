#ifndef ANNOTATION_REACTIVE_H
#define ANNOTATION_REACTIVE_H

// src/annotation/reactive.h — reactive-binding marker.
//
// REACTIVE(text): identify the reactive object a field or variable is bound to,
// so a reader (or an agent) sees which reactive drives the value without
// hunting for the bind call. The text is the reactive object name, e.g.
//
//     ;;REACTIVE("healthReactive")
//     void *health;
//
// See intention.h for the macro convention (C has no language-level
// annotations). Usage: `;;REACTIVE("...")` (see preferences.md, Two-Semicolon
// Annotation Style Law).

#define REACTIVE(text) _Static_assert(1, "@Reactive(" text ")");

#endif
