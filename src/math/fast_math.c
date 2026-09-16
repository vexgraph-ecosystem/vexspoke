#include "math/fast_math.h"

#include <string.h>

#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: FastMath (math/fast_math.c — defined in math/fast_math.h)
 * LEVEL: L2 — Behavior (relaxed approximations deliberately trading IEEE conformance for speed)
 * ============================================================================
 * High-speed mathematical approximations for real-time 3D, procedural animations,
 * particles, and graphics. Replaces expensive libm routines with branchless
 * bit manipulations and low-order polynomial / rational approximations.
 * ============================================================================
 */

float FastMath_invSqrt(float x) {
    if (x <= 0.0f)
        return 0.0f;
    float xhalf = 0.5f * x;
    uint32_t i;
    memcpy(&i, &x, 4);
    i = 0x5f3759dfu - (i >> 1);
    memcpy(&x, &i, 4);
    x = x * (1.5f - xhalf * x * x);
    return x;
}

float FastMath_inv(float x) {
    if (x == 0.0f)
        return 0.0f;
    // Single Newton-Raphson refinement on IEEE exponent reciprocal
    uint32_t i;
    memcpy(&i, &x, 4);
    i = 0x7ef311c2u - i;
    float y;
    memcpy(&y, &i, 4);
    return y * (2.0f - x * y);
}

static inline float wrap_pi(float x) {
    // Wrap into [-PI, PI]
    while (x > FAST_MATH_PI)
        x -= FAST_MATH_TWO_PI;
    while (x < -FAST_MATH_PI)
        x += FAST_MATH_TWO_PI;
    return x;
}

float FastMath_sin(float x) {
    x = wrap_pi(x);
    // Bhaskara I formula on [-PI, PI]:
    // For x in [0, PI]: 16*x*(PI - x) / (5*PI^2 - 4*x*(PI - x))
    if (x >= 0.0f) {
        float num = 16.0f * x * (FAST_MATH_PI - x);
        float den = 5.0f * FAST_MATH_PI * FAST_MATH_PI - 4.0f * x * (FAST_MATH_PI - x);
        return num / den;
    } else {
        float pos = -x;
        float num = 16.0f * pos * (FAST_MATH_PI - pos);
        float den = 5.0f * FAST_MATH_PI * FAST_MATH_PI - 4.0f * pos * (FAST_MATH_PI - pos);
        return -(num / den);
    }
}

float FastMath_cos(float x) {
    return FastMath_sin(x + FAST_MATH_HALF_PI);
}

float FastMath_tan(float x) {
    float c = FastMath_cos(x);
    if (c > -1e-6f && c < 1e-6f)
        return 0.0f;
    return FastMath_sin(x) / c;
}

float FastMath_atan(float x) {
    // Fast minimax polynomial approximation on [-1, 1] with range reduction
    if (x > 1.0f) {
        return FAST_MATH_HALF_PI - FastMath_atan(1.0f / x);
    }
    if (x < -1.0f) {
        return -FAST_MATH_HALF_PI - FastMath_atan(1.0f / x);
    }
    // P(x) = x * (1 - 0.3333 * x^2)
    float x2 = x * x;
    return x * (0.999866f + x2 * (-0.3302995f + x2 * (0.180141f - 0.085133f * x2)));
}

float FastMath_atan2(float y, float x) {
    if (x == 0.0f) {
        if (y > 0.0f) return FAST_MATH_HALF_PI;
        if (y < 0.0f) return -FAST_MATH_HALF_PI;
        return 0.0f;
    }
    float a = FastMath_atan(y / x);
    if (x < 0.0f) {
        if (y >= 0.0f) return a + FAST_MATH_PI;
        return a - FAST_MATH_PI;
    }
    return a;
}

float FastMath_abs(float x) {
    uint32_t i;
    memcpy(&i, &x, 4);
    i &= 0x7FFFFFFFu;
    memcpy(&x, &i, 4);
    return x;
}

float FastMath_round(float x) {
    if (x >= 0.0f) {
        float f = x + 16384.0f;
        return f - 16384.0f;
    } else {
        float f = x - 16384.0f;
        return f + 16384.0f;
    }
}

float FastMath_clamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

float FastMath_lerp(float a, float b, float t) {
    return a + t * (b - a);
}

bool FastMath_approxEqual(float a, float b, float epsilon) {
    float diff = FastMath_abs(a - b);
    return diff <= epsilon;
}

float FastMath_toRadians(float deg) {
    return deg * FAST_MATH_DEG_TO_RAD;
}

float FastMath_toDegrees(float rad) {
    return rad * FAST_MATH_RAD_TO_DEG;
}
