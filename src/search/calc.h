#ifndef SEARCH_CALC_H
#define SEARCH_CALC_H

#include <stdbool.h>
#include <stddef.h>

// search/calc.h — String Mathematical Expression Calculation Engine.
//
// Single Class Per File Law: Calc.
//
// Evaluates arbitrary math expressions from strings (e.g. "sin(rad(90)) * atan(34) * pi")
// directly into double/float scalar values. Allocation-free Pratt precedence parser.
//
// Supported operators:
//   +, -, *, /, %, ^ (power), unary -
//
// Supported functions:
//   sin, cos, tan, asin, acos, atan, atan2, rad, deg, sqrt, invsqrt,
//   abs, round, floor, ceil, min, max, clamp, pow, exp, log
//
// Supported constants:
//   pi, e, tau

// Evaluates an expression string into a double result. Returns true on success, false on syntax error.
bool Calc_eval(const char *expr, double *outValue);

// Evaluates into a float result.
bool Calc_evalFloat(const char *expr, float *outValue);

// Convenience: returns value directly, or fallback upon error.
double Calc_evalWithFallback(const char *expr, double fallback);

#endif
