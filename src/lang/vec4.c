#include "lang/vec4.h"

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Vec4
 * ============================================================================
 * Top-level umbrella forwarder for the Vec4 family: re-exports
 * lang/vec4/vec4.h and lang/vec4/vec4d.h so consumers include one header and
 * reach both the single- and double-precision 4D vector classes. Exists
 * purely for include ergonomics — the Single Class Per File Law keeps the
 * real implementations in lang/vec4/vec4.c and lang/vec4/vec4d.c. Memory:
 * none — this file owns no structs, no functions, no state. Lifetime:
 * compile-time only.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Vec4 (lang/vec4.c — forwarded to lang/vec4/vec4.c)
 * LEVEL: L2 — Behavior (top-level vec4 umbrella)
 * ============================================================================
 * Implementation lives in src/lang/vec4/vec4.c per the Single Class Per File Law.
 *
 * STRUCT FIELDS: none — procedural (forwarder; implementation lives in lang/vec4/vec4.c)
 *
 * FUNCTION REGISTRY: none — procedural forwarder (no functions defined in this file)
 * ============================================================================
 */
