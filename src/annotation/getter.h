#ifndef ANNOTATION_GETTER_H
#define ANNOTATION_GETTER_H

// src/annotation/getter.h — Field getter annotation marker.
//
// Explicitly marks a getter function or getter block in class implementations.
//
// Usage: prefix the marker with two semicolons:
// `;;GETTER` (see preferences.md, Two-Semicolon Annotation Style Law).

#define GETTER _Static_assert(1, "@Getter");

#endif
