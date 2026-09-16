#ifndef MATH_FAST_MATH_H
#define MATH_FAST_MATH_H

#include <stdbool.h>
#include <stdint.h>

// math/fast_math.h — Relaxed Fast Math Engine (Violates IEEE 754 for Speed).
//
// Single Class Per File Law: FastMath.
//
// High-performance polynomial and bitwise approximations tailored for real-time
// 3D graphics, particle systems, vertex shaders, and game logic. Intentionally
// relaxes IEEE 754 bit-exactness to eliminate division, square root, and
// trigonometric latency.

#define FAST_MATH_PI         3.1415927f
#define FAST_MATH_HALF_PI    1.5707964f
#define FAST_MATH_TWO_PI     6.2831855f
#define FAST_MATH_EPSILON    0.0001f
#define FAST_MATH_DEG_TO_RAD 0.0174532925f
#define FAST_MATH_RAD_TO_DEG 57.2957795f

// Quake III 0x5f3759df fast inverse square root (1 Newton-Raphson pass).
float FastMath_invSqrt(float x);

// Fast reciprocal approximation.
float FastMath_inv(float x);

// Bhaskara I rational polynomial sine (radians, wraps any periodic domain).
float FastMath_sin(float x);

// Bhaskara I rational polynomial cosine.
float FastMath_cos(float x);

// Fast tangent approximation.
float FastMath_tan(float x);

// Fast polynomial arc tangent.
float FastMath_atan(float x);
float FastMath_atan2(float y, float x);

// Branchless absolute value (IEEE sign-bit clear).
float FastMath_abs(float x);

// Magic-float fast rounding (16384 trick).
float FastMath_round(float x);

// Branchless clamp.
float FastMath_clamp(float val, float min, float max);

// Linear interpolation.
float FastMath_lerp(float a, float b, float t);

// Tolerance-based approximate equality.
bool FastMath_approxEqual(float a, float b, float epsilon);

float FastMath_toRadians(float deg);
float FastMath_toDegrees(float rad);

#endif
