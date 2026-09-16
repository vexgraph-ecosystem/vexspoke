#ifndef MATH_MATH_H
#define MATH_MATH_H

#include <stdbool.h>
#include <stdint.h>

#include "math/strict_math.h"
#include "math/fast_math.h"

// math/math.h — Unified Math Facade.
//
// Single Class Per File Law: Math.
//
// Unifies the Strict IEEE 754 precision math engine with the Relaxed Fast Math
// approximations under a single consistent API.
//
// DOCTRINE:
//   - Functions without the "fast_" prefix are STRICT (exact IEEE 754 precision by default).
//   - Functions with the "fast_" prefix are RELAXED APPROXIMATIONS (violates IEEE 754 for speed).

#define MATH_PI         STRICT_MATH_PI
#define MATH_HALF_PI    STRICT_MATH_HALF_PI
#define MATH_TWO_PI     STRICT_MATH_TWO_PI
#define MATH_E          STRICT_MATH_E
#define MATH_TAU        STRICT_MATH_TAU
#define MATH_DEG_TO_RAD STRICT_MATH_DEG_TO_RAD
#define MATH_RAD_TO_DEG STRICT_MATH_RAD_TO_DEG

// Strict by default (Exact IEEE 754 precision)
float Math_sin(float x);
float Math_cos(float x);
float Math_tan(float x);
float Math_asin(float x);
float Math_acos(float x);
float Math_atan(float x);
float Math_atan2(float y, float x);
float Math_sqrt(float x);
float Math_invSqrt(float x);
float Math_pow(float base, float exp);
float Math_exp(float x);
float Math_log(float x);
float Math_abs(float x);
float Math_floor(float x);
float Math_ceil(float x);
float Math_round(float x);
float Math_clamp(float val, float min, float max);
float Math_lerp(float a, float b, float t);
float Math_toRadians(float deg);
float Math_toDegrees(float rad);

// Double precision strict
double Math_sinD(double x);
double Math_cosD(double x);
double Math_sqrtD(double x);
double Math_atan2D(double y, double x);

// Fast approximations (deliberately relaxes IEEE 754 for real-time 3D/graphics)
float Math_fast_sin(float x);
float Math_fast_cos(float x);
float Math_fast_tan(float x);
float Math_fast_atan(float x);
float Math_fast_atan2(float y, float x);
float Math_fast_invSqrt(float x);
float Math_fast_inv(float x);
float Math_fast_abs(float x);
float Math_fast_round(float x);
float Math_fast_clamp(float val, float min, float max);
float Math_fast_lerp(float a, float b, float t);
bool  Math_fast_approxEqual(float a, float b, float epsilon);

#endif
