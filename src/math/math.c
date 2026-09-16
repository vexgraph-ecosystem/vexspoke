#include "math/math.h"

#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Math (math/math.c — defined in math/math.h)
 * LEVEL: L2 — Behavior (unified math facade)
 * ============================================================================
 * Dispatches math operations according to the dual precision doctrine:
 *   - No fast prefix: strictly IEEE 754 precision via StrictMath.
 *   - "fast_" prefix: relaxed polynomial / bitwise approximations via FastMath.
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
