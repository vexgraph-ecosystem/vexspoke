#ifndef ANNOTATION_INHERITS_H
#define ANNOTATION_INHERITS_H

// src/annotation/inherits.h — the inheritance (extends) marker.
//
// INHERITS(text): name the base class a class extends — vex's `extends`
// keyword. The text is the base class name, e.g.
//
//     ;;INHERITS("Widget")
//     typedef struct Button { ... } Button;
//
// Runtime parent chains still resolve through Type_getParentClass / Type_isA
// (the One Type Registry Law); this marker documents the compile-time intent
// at the class site so the chain reads from the file alone.
//
// See intention.h for the macro convention (C has no language-level
// annotations). Usage: `;;INHERITS("Base")` (see preferences.md, Two-Semicolon
// Annotation Style Law).

#define INHERITS(text) _Static_assert(1, "@Inherits(" text ")");

#endif
