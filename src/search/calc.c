#include "search/calc.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math/strict_math.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Calc (search/calc.c — defined in search/calc.h)
 * LEVEL: L2 — Behavior (allocation-free string math expression calculator)
 * ============================================================================
 * Fast recursive-descent math parser supporting operator precedence, constants,
 * and scientific / trigonometric functions. Evaluates in-place on the stack.
 * ============================================================================
 */

typedef struct CalcParser {
    const char *p;
    bool hasError;
} CalcParser;

static void skip_whitespace(CalcParser *cp) {
    while (*(*cp).p != '\0' && isspace((unsigned char)*(*cp).p)) {
        (*cp).p++;
    }
}

static double parse_expression(CalcParser *cp);

static double parse_primary(CalcParser *cp) {
    skip_whitespace(cp);
    if (*(*cp).p == '\0') {
        (*cp).hasError = true;
        return 0.0;
    }

    // Unary plus and minus
    if (*(*cp).p == '+') {
        (*cp).p++;
        return parse_primary(cp);
    }
    if (*(*cp).p == '-') {
        (*cp).p++;
        return -parse_primary(cp);
    }

    // Parenthesized sub-expression
    if (*(*cp).p == '(') {
        (*cp).p++;
        double val = parse_expression(cp);
        skip_whitespace(cp);
        if (*(*cp).p == ')') {
            (*cp).p++;
        } else {
            (*cp).hasError = true;
        }
        return val;
    }

    // Number literal
    if (isdigit((unsigned char)*(*cp).p) || *(*cp).p == '.') {
        char *endptr = NULL;
        double val = strtod((*cp).p, &endptr);
        if (endptr == (*cp).p) {
            (*cp).hasError = true;
            return 0.0;
        }
        (*cp).p = endptr;
        return val;
    }

    // Identifier: constant or function
    if (isalpha((unsigned char)*(*cp).p) || *(*cp).p == '_') {
        char name[32];
        size_t n = 0;
        while ((isalnum((unsigned char)*(*cp).p) || *(*cp).p == '_') && n < sizeof(name) - 1) {
            name[n++] = (char) tolower((unsigned char)*(*cp).p);
            (*cp).p++;
        }
        name[n] = '\0';
        skip_whitespace(cp);

        // Check constants
        if (strcmp(name, "pi") == 0) return STRICT_MATH_PI;
        if (strcmp(name, "e") == 0) return STRICT_MATH_E;
        if (strcmp(name, "tau") == 0) return STRICT_MATH_TAU;

        // Function call: expect '('
        if (*(*cp).p != '(') {
            (*cp).hasError = true;
            return 0.0;
        }
        (*cp).p++; // consume '('

        // Parse arguments
        double arg1 = parse_expression(cp);
        double arg2 = 0.0;
        double arg3 = 0.0;
        skip_whitespace(cp);
        if (*(*cp).p == ',') {
            (*cp).p++;
            arg2 = parse_expression(cp);
            skip_whitespace(cp);
            if (*(*cp).p == ',') {
                (*cp).p++;
                arg3 = parse_expression(cp);
                skip_whitespace(cp);
            }
        }
        if (*(*cp).p == ')') {
            (*cp).p++;
        } else {
            (*cp).hasError = true;
            return 0.0;
        }

        // Evaluate function
        if (strcmp(name, "sin") == 0) return sin(arg1);
        if (strcmp(name, "cos") == 0) return cos(arg1);
        if (strcmp(name, "tan") == 0) return tan(arg1);
        if (strcmp(name, "asin") == 0) return asin(arg1);
        if (strcmp(name, "acos") == 0) return acos(arg1);
        if (strcmp(name, "atan") == 0) return atan(arg1);
        if (strcmp(name, "atan2") == 0) return atan2(arg1, arg2);
        if (strcmp(name, "rad") == 0) return arg1 * STRICT_MATH_DEG_TO_RAD;
        if (strcmp(name, "deg") == 0) return arg1 * STRICT_MATH_RAD_TO_DEG;
        if (strcmp(name, "sqrt") == 0) return arg1 >= 0.0 ? sqrt(arg1) : 0.0;
        if (strcmp(name, "invsqrt") == 0) return arg1 > 0.0 ? 1.0 / sqrt(arg1) : 0.0;
        if (strcmp(name, "abs") == 0) return fabs(arg1);
        if (strcmp(name, "round") == 0) return round(arg1);
        if (strcmp(name, "floor") == 0) return floor(arg1);
        if (strcmp(name, "ceil") == 0) return ceil(arg1);
        if (strcmp(name, "min") == 0) return arg1 < arg2 ? arg1 : arg2;
        if (strcmp(name, "max") == 0) return arg1 > arg2 ? arg1 : arg2;
        if (strcmp(name, "clamp") == 0) return StrictMath_clampD(arg1, arg2, arg3);
        if (strcmp(name, "pow") == 0) return pow(arg1, arg2);
        if (strcmp(name, "exp") == 0) return exp(arg1);
        if (strcmp(name, "log") == 0) return arg1 > 0.0 ? log(arg1) : 0.0;

        (*cp).hasError = true;
        return 0.0;
    }

    (*cp).hasError = true;
    return 0.0;
}

static double parse_factor(CalcParser *cp) {
    double base = parse_primary(cp);
    skip_whitespace(cp);
    while (*(*cp).p == '^') {
        (*cp).p++;
        double expVal = parse_factor(cp); // right-associative
        base = pow(base, expVal);
        skip_whitespace(cp);
    }
    return base;
}

static double parse_term(CalcParser *cp) {
    double left = parse_factor(cp);
    skip_whitespace(cp);
    while (*(*cp).p == '*' || *(*cp).p == '/' || *(*cp).p == '%') {
        char op = *(*cp).p++;
        double right = parse_factor(cp);
        if (op == '*') {
            left *= right;
        } else if (op == '/') {
            if (right == 0.0) {
                (*cp).hasError = true;
                return 0.0;
            }
            left /= right;
        } else if (op == '%') {
            if (right == 0.0) {
                (*cp).hasError = true;
                return 0.0;
            }
            left = fmod(left, right);
        }
        skip_whitespace(cp);
    }
    return left;
}

static double parse_expression(CalcParser *cp) {
    double left = parse_term(cp);
    skip_whitespace(cp);
    while (*(*cp).p == '+' || *(*cp).p == '-') {
        char op = *(*cp).p++;
        double right = parse_term(cp);
        if (op == '+') {
            left += right;
        } else {
            left -= right;
        }
        skip_whitespace(cp);
    }
    return left;
}

bool Calc_eval(const char *expr, double *outValue) {
    if (!expr || !outValue) return false;
    CalcParser cp = { .p = expr, .hasError = false };
    skip_whitespace(&cp);
    if (*cp.p == '\0') return false;

    double res = parse_expression(&cp);
    skip_whitespace(&cp);
    if (cp.hasError || *cp.p != '\0') {
        return false;
    }
    *outValue = res;
    return true;
}

bool Calc_evalFloat(const char *expr, float *outValue) {
    if (!outValue) return false;
    double d = 0.0;
    if (!Calc_eval(expr, &d)) return false;
    *outValue = (float) d;
    return true;
}

double Calc_evalWithFallback(const char *expr, double fallback) {
    double d = 0.0;
    if (Calc_eval(expr, &d)) return d;
    return fallback;
}
