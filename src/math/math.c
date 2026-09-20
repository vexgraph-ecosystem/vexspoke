#include "math/math.h"

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Math
 * ============================================================================
 * Unified math facade dispatching every operation to one of two engines per
 * the dual precision doctrine: no prefix means strictly IEEE 754 precision via
 * StrictMath, while the "fast_" prefix means relaxed polynomial/bitwise
 * approximations via FastMath. Pure procedural — no state, no allocation, no
 * struct; every function is a thin named dispatch so callers never include
 * the engine headers directly.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Math (math/math.c — defined in math/math.h)
 * LEVEL: L2 — Behavior (unified math facade)
 * ============================================================================
 * Dispatches math operations according to the dual precision doctrine:
 *   - No fast prefix: strictly IEEE 754 precision via StrictMath.
 *   - "fast_" prefix: relaxed polynomial / bitwise approximations via FastMath.
 *
 * STRUCT FIELDS: none — procedural (operates on scalar float/double operands;
 * no state)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - Math_sin(x) / Math_cos(x) / Math_tan(x)
 *   - Math_asin(x) / Math_acos(x) / Math_atan(x) / Math_atan2(y, x)
 *   - Math_sqrt(x) / Math_invSqrt(x) / Math_pow(base, exp)
 *   - Math_exp(x) / Math_log(x) / Math_abs(x)
 *   - Math_floor(x) / Math_ceil(x) / Math_round(x)
 *   - Math_clamp(val, min, max) / Math_lerp(a, b, t)
 *   - Math_toRadians(deg) / Math_toDegrees(rad)
 *   - Math_sinD(x) / Math_cosD(x) / Math_sqrtD(x) / Math_atan2D(y, x)
 *   - Math_fast_sin(x) / Math_fast_cos(x) / Math_fast_tan(x)
 *   - Math_fast_atan(x) / Math_fast_atan2(y, x)
 *   - Math_fast_invSqrt(x) / Math_fast_inv(x)
 *   - Math_fast_abs(x) / Math_fast_round(x)
 *   - Math_fast_clamp(val, min, max) / Math_fast_lerp(a, b, t)
 *   - Math_fast_approxEqual(a, b, epsilon)
 * ============================================================================
 */

float Math_sin(float x) {
    return StrictMath_sin(x);
}

float Math_cos(float x) {
    return StrictMath_cos(x);
}

float Math_tan(float x) {
    return StrictMath_tan(x);
}

float Math_asin(float x) {
    return StrictMath_asin(x);
}

float Math_acos(float x) {
    return StrictMath_acos(x);
}

float Math_atan(float x) {
    return StrictMath_atan(x);
}

float Math_atan2(float y, float x) {
    return StrictMath_atan2(y, x);
}

float Math_sqrt(float x) {
    return StrictMath_sqrt(x);
}

float Math_invSqrt(float x) {
    return StrictMath_invSqrt(x);
}

float Math_pow(float base, float exp) {
    return StrictMath_pow(base, exp);
}

float Math_exp(float x) {
    return StrictMath_exp(x);
}

float Math_log(float x) {
    return StrictMath_log(x);
}

float Math_abs(float x) {
    return StrictMath_abs(x);
}

float Math_floor(float x) {
    return StrictMath_floor(x);
}

float Math_ceil(float x) {
    return StrictMath_ceil(x);
}

float Math_round(float x) {
    return StrictMath_round(x);
}

float Math_clamp(float val, float min, float max) {
    return StrictMath_clamp(val, min, max);
}

float Math_lerp(float a, float b, float t) {
    return StrictMath_lerp(a, b, t);
}

float Math_toRadians(float deg) {
    return StrictMath_toRadians(deg);
}

float Math_toDegrees(float rad) {
    return StrictMath_toDegrees(rad);
}

double Math_sinD(double x) {
    return StrictMath_sinD(x);
}

double Math_cosD(double x) {
    return StrictMath_cosD(x);
}

double Math_sqrtD(double x) {
    return StrictMath_sqrtD(x);
}

double Math_atan2D(double y, double x) {
    return StrictMath_atan2D(y, x);
}

float Math_fast_sin(float x) {
    return FastMath_sin(x);
}

float Math_fast_cos(float x) {
    return FastMath_cos(x);
}

float Math_fast_tan(float x) {
    return FastMath_tan(x);
}

float Math_fast_atan(float x) {
    return FastMath_atan(x);
}

float Math_fast_atan2(float y, float x) {
    return FastMath_atan2(y, x);
}

float Math_fast_invSqrt(float x) {
    return FastMath_invSqrt(x);
}

float Math_fast_inv(float x) {
    return FastMath_inv(x);
}

float Math_fast_abs(float x) {
    return FastMath_abs(x);
}

float Math_fast_round(float x) {
    return FastMath_round(x);
}

float Math_fast_clamp(float val, float min, float max) {
    return FastMath_clamp(val, min, max);
}

float Math_fast_lerp(float a, float b, float t) {
    return FastMath_lerp(a, b, t);
}

bool Math_fast_approxEqual(float a, float b, float epsilon) {
    return FastMath_approxEqual(a, b, epsilon);
}
