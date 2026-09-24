#ifndef ANNOTATION_INHERITS_H
#define ANNOTATION_INHERITS_H

// src/annotation/inherits.h — the inheritance (identity) marker.
//
// INHERITS(text): name the identity-defining classes/interfaces a class is
// built out of — the "without these, this thing would not be itself" set,
// comma-separated, e.g.
//
//     ;;INHERITS("GraphicsPanel, Reactive")
//     typedef struct Button { ... } Button;
//
// It is not a primitive type (never ;;INHERITS("uint64_t")) and not a
// collection (a List does not mean you inherit []). It names the contracts
// that make the class exist: a reader sees at a glance what the class is made
// of, and which interfaces it must satisfy. A note only — the runtime parent
// chain still resolves through Type_getParentClass / Type_isA (the One Type
// Registry Law); this marker documents the compile-time intent so the class's
// makeup reads from the file alone.
//
// See intention.h for the macro convention (C has no language-level
// annotations). Usage: `;;INHERITS("...")` (see preferences.md, Two-Semicolon
// Annotation Style Law).

#define INHERITS(text) _Static_assert(1, "@Inherits(" text ")");

#endif
