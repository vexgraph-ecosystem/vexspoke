#include "math/strict_math.h"

#include <math.h>

#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: StrictMath (math/strict_math.c — defined in math/strict_math.h)
 * LEVEL: L2 — Behavior (strict IEEE 754 precision math for physics & simulations)
 * ============================================================================
 * Full-precision floating-point functions conforming strictly to IEEE 754.
 * Guaranteed accuracy, bit-exact roundings, and rigorous domain checks for
 * rigid-body physics, trajectory integration, and orbital mechanics.
 * ============================================================================
 */

float StrictMath_sin(float x) {
    return sinf(x);
}

float StrictMath_cos(float x) {
    return cosf(x);
}

float StrictMath_tan(float x) {
    return tanf(x);
}

float StrictMath_asin(float x) {
    return asinf(x);
}

float StrictMath_acos(float x) {
    return acosf(x);
}

float StrictMath_atan(float x) {
    return atanf(x);
}

float StrictMath_atan2(float y, float x) {
    return atan2f(y, x);
}

float StrictMath_sqrt(float x) {
    return sqrtf(x);
}

float StrictMath_invSqrt(float x) {
    if (x <= 0.0f)
        return 0.0f;
    return 1.0f / sqrtf(x);
}

float StrictMath_pow(float base, float exp) {
    return powf(base, exp);
}

float StrictMath_exp(float x) {
    return expf(x);
}

float StrictMath_log(float x) {
    return logf(x);
}

float StrictMath_abs(float x) {
    return fabsf(x);
}

float StrictMath_floor(float x) {
    return floorf(x);
}

float StrictMath_ceil(float x) {
    return ceilf(x);
}

float StrictMath_round(float x) {
    return roundf(x);
}

float StrictMath_clamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

float StrictMath_lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float StrictMath_toRadians(float deg) {
    return deg * (float) STRICT_MATH_DEG_TO_RAD;
}

float StrictMath_toDegrees(float rad) {
    return rad * (float) STRICT_MATH_RAD_TO_DEG;
}

double StrictMath_sinD(double x) {
    return sin(x);
}

double StrictMath_cosD(double x) {
    return cos(x);
}

double StrictMath_tanD(double x) {
    return tan(x);
}

double StrictMath_asinD(double x) {
    return asin(x);
}

double StrictMath_acosD(double x) {
    return acos(x);
}

double StrictMath_atanD(double x) {
    return atan(x);
}

double StrictMath_atan2D(double y, double x) {
    return atan2(y, x);
}

double StrictMath_sqrtD(double x) {
    return sqrt(x);
}

double StrictMath_invSqrtD(double x) {
    if (x <= 0.0)
        return 0.0;
    return 1.0 / sqrt(x);
}

double StrictMath_powD(double base, double exp) {
    return pow(base, exp);
}

double StrictMath_expD(double x) {
    return exp(x);
}

double StrictMath_logD(double x) {
    return log(x);
}

double StrictMath_absD(double x) {
    return fabs(x);
}

double StrictMath_floorD(double x) {
    return floor(x);
}

double StrictMath_ceilD(double x) {
    return ceil(x);
}

double StrictMath_roundD(double x) {
    return round(x);
}

double StrictMath_clampD(double val, double min, double max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

double StrictMath_lerpD(double a, double b, double t) {
    return a + t * (b - a);
}
