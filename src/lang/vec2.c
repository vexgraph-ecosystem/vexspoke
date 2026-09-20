#include "lang/vec2.h"

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Vec2 (forwarding shim)
 * ============================================================================
 * Top-level umbrella translation unit for the Vec2 family: per the Single
 * Class Per File Law the canonical implementation lives in
 * src/lang/vec2/vec2.c, and this file exists only so the umbrella header
 * lang/vec2.h has a matching .c pair. It owns no structs and no functions;
 * consumers include lang/vec2.h and link against lang/vec2/vec2.c.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Vec2 (lang/vec2.c — forwarded to lang/vec2/vec2.c)
 * LEVEL: L2 — Behavior (top-level vec2 umbrella)
 * ============================================================================
 * Implementation lives in src/lang/vec2/vec2.c per the Single Class Per File Law.
 *
 * STRUCT FIELDS (Mirroring lang/vec2/vec2.h):
 * ----------------------------------------------------------------------------
 *   Vec2 {
 *     union {
 *       struct { float horizontal; float vertical; };
 *       struct { float x; float y; };
 *       struct { float right; float up; };
 *       float data[2];
 *     };
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * none — forwarding shim; every Vec2_* function lives in lang/vec2/vec2.c
 * ============================================================================
 */
