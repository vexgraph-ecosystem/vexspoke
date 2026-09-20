#include "lang/vec3.h"

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Vec3
 * ============================================================================
 * Top-level umbrella forwarder for the Vec3 family. Per the Single Class Per
 * File Law the canonical implementation lives in lang/vec3/vec3.c (plus the
 * vec3d / vec3_int_float / vec3_long_double variants); this file exists only
 * so consumers can include "lang/vec3.h" and get the whole family. It owns
 * no struct and defines no functions.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Vec3 (lang/vec3.c — forwarded to lang/vec3/vec3.c)
 * LEVEL: L2 — Behavior (top-level vec3 umbrella)
 * ============================================================================
 * Implementation lives in src/lang/vec3/vec3.c per the Single Class Per File Law.
 *
 * STRUCT FIELDS: none — procedural (forwarder; the Vec3 family structs live in lang/vec3/vec3.h and siblings)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - (none — forwarder; all Vec3 functions live in lang/vec3/vec3.c and siblings)
 * ============================================================================
 */
