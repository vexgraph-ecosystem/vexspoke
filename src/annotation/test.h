#ifndef ANNOTATION_TEST_H
#define ANNOTATION_TEST_H

// src/annotation/test.h — the test/inspection-affordance marker.
//
// TEST: mark a function or field that exists so a TEST (or a tool) can observe
// or drive a class — a getter for internal state, a probe, a reset hook. It is a
// test/inspection affordance, not a production necessity: production code does
// not need it to compute a value or do its work. A `;;TEST` surface is normally
// still compiled in and may be used by tools or curious callers, but no
// production code path may depend on it for correctness — a `;;TEST` function
// that becomes load-bearing is a mis-annotation; promote it to a real API.
//
// This is C-only in spelling: Rust already reserves `#[test]` for its own
// harness, so the Rust form is the built-in attribute, not a proc-macro. That
// follows the Two-Semicolon Annotation Style Law's rule that REACTIVE and
// INHERITS are C-only where Rust has no equivalent.
//
// It takes no text — the marker is the whole statement. `grep -rn ';;TEST'`
// lists every test/inspection affordance in the tree, in both `src/` and the
// shared `tests/` repository (the marker header is on the test include path).
//
// See intention.h for the macro convention (C has no language-level
// annotations). Usage: `;;TEST` on the line above the declaration.

#define TEST _Static_assert(1, "@Test");

#endif
