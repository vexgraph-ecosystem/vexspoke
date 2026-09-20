#include "primitive/pack.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Pack
 * ============================================================================
 * Bit-packing utilities (Legacy: primitive/Pack.java): pairs of bytes, shorts,
 * and ints packed into single wider words and unpacked back out. Stateless —
 * the header ships inline helpers and this .c carries no state and no
 * functions of its own. Lives at R2 as a leaf primitive behavior.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Pack (primitive/pack.c)
 * LEVEL: L2 — Behavior (primitive behavior API)
 * ============================================================================
 * bit-packing utilities (Legacy: primitive/Pack.java).
 *
 * STRUCT FIELDS: none — procedural (operates on packed primitive payload buffers)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - Pack_packByte(b1, b2)
 *   - Pack_packShort(s1, s2)
 *   - Pack_packInt(i1, i2)
 *   - Pack_unpackByte1(p)
 *   - Pack_unpackByte2(p)
 *   - Pack_unpackShort1(p)
 *   - Pack_unpackShort2(p)
 *   - Pack_unpackInt1(p)
 *   - Pack_unpackInt2(p)
 * ============================================================================
 */

// pack.c — no state, header-only inline helpers.
