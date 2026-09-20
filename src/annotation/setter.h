#ifndef ANNOTATION_SETTER_H
#define ANNOTATION_SETTER_H

// src/annotation/setter.h — Field setter annotation marker.
//
// Explicitly marks a setter function or setter block in class implementations.
//
// Usage: prefix the marker with two semicolons:
// `;;SETTER` (see preferences.md, Two-Semicolon Annotation Style Law).

#define SETTER _Static_assert(1, "@Setter");

#endif
