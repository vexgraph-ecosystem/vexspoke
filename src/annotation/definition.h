#ifndef ANNOTATION_DEFINITION_H
#define ANNOTATION_DEFINITION_H

// src/annotation/definition.h — Class architectural definition marker.
//
// Marks the deep architectural specification of a class: raison d'être,
// memory layout, relational invariants, operational mechanics, and edge cases.
// Complements `;;OVERVIEW` (which serves as the structural summary and function index).
//
// Usage: prefix the marker with two semicolons:
// `;;DEFINITION` (see preferences.md, Two-Semicolon Annotation Style Law).

#define DEFINITION _Static_assert(1, "@Definition");

#endif
