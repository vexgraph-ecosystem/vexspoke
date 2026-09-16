#ifndef MATH_STRICT_MATH_H
#define MATH_STRICT_MATH_H

#include <stdbool.h>
#include <stdint.h>

// math/strict_math.h — Strict IEEE 754 Math Engine.
//
// Single Class Per File Law: StrictMath.
//
// Full-precision, strictly IEEE 754-compliant floating-point routines for
// physics simulations, celestial mechanics, exact trajectory integration,
// and collision manifolds. Guaranteed bit-level conformance and error bounds.

#define STRICT_MATH_PI         3.14159265358979323846
#define STRICT_MATH_HALF_PI    1.57079632679489661923
#define STRICT_MATH_TWO_PI     6.28318530717958647692
#define STRICT_MATH_E          2.71828182845904523536
#define STRICT_MATH_TAU        6.28318530717958647692
#define STRICT_MATH_DEG_TO_RAD 0.01745329251994329577
#define STRICT_MATH_RAD_TO_DEG 57.2957795130823208768

// Single-precision strict routines
float StrictMath_sin(float x);
float StrictMath_cos(float x);
float StrictMath_tan(float x);
float StrictMath_asin(float x);
float StrictMath_acos(float x);
float StrictMath_atan(float x);
float StrictMath_atan2(float y, float x);
float StrictMath_sqrt(float x);
float StrictMath_invSqrt(float x);
float StrictMath_pow(float base, float exp);
float StrictMath_exp(float x);
float StrictMath_log(float x);
float StrictMath_abs(float x);
float StrictMath_floor(float x);
float StrictMath_ceil(float x);
float StrictMath_round(float x);
float StrictMath_clamp(float val, float min, float max);
float StrictMath_lerp(float a, float b, float t);
float StrictMath_toRadians(float deg);
float StrictMath_toDegrees(float rad);

// Double-precision strict routines for celestial & large-world coordinates
double StrictMath_sinD(double x);
double StrictMath_cosD(double x);
double StrictMath_tanD(double x);
double StrictMath_asinD(double x);
double StrictMath_acosD(double x);
double StrictMath_atanD(double x);
double StrictMath_atan2D(double y, double x);
double StrictMath_sqrtD(double x);
double StrictMath_invSqrtD(double x);
double StrictMath_powD(double base, double exp);
double StrictMath_expD(double x);
double StrictMath_logD(double x);
double StrictMath_absD(double x);
double StrictMath_floorD(double x);
double StrictMath_ceilD(double x);
double StrictMath_roundD(double x);
double StrictMath_clampD(double val, double min, double max);
double StrictMath_lerpD(double a, double b, double t);

#endif
