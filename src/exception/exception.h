#ifndef EXCEPTION_EXCEPTION_H
#define EXCEPTION_EXCEPTION_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "c23/constructor.h"
#include "oop/type.h"

// Categories of runtime exceptions
typedef enum ExceptionCategory {
    EXCEPTION_RUNTIME = 0,
    EXCEPTION_WINDOW,
    EXCEPTION_GRAPHICS,
    EXCEPTION_NULL_POINTER,
    EXCEPTION_ILLEGAL_STATE,
    EXCEPTION_IO,
    EXCEPTION_OUT_OF_MEMORY
} ExceptionCategory;

typedef struct Exception {
    uint64_t type;              // TYPE_EXCEPTION_SINGLETON
    ExceptionCategory category;
    const char *site;           // e.g. "GraphicsLoop::setGraphicsLoop"
    const char *file;           // __FILE__
    int line;                   // __LINE__
    char *message;              // Dynamically allocated diagnostic message (0..N, no hardcoded capacity)
    char *details;              // Dynamically allocated variable / context dump (0..N, no hardcoded capacity)
} Exception;

// Construct a diagnostic record. The empty form has no allocated strings;
// formatted forms own their message until Exception_free.
Exception Exception_0(void);
Exception Exception_5(ExceptionCategory category, const char *site,
                      const char *file, int line, const char *fmt, ...);
Exception Exception_6(ExceptionCategory category, const char *site,
                      const char *file, int line, const char *fmt, ...);
Exception Exception_7(ExceptionCategory category, const char *site,
                      const char *file, int line, const char *fmt, ...);
Exception Exception_8(ExceptionCategory category, const char *site,
                      const char *file, int line, const char *fmt, ...);
#define Exception(...) CONSTRUCTOR_DISPATCH(Exception, __VA_ARGS__)

// Append or set context/variable details
void Exception_setDetails(Exception *self, const char *fmt, ...);
void Exception_setDetailsV(Exception *self, const char *fmt, va_list args);

// Free dynamically allocated diagnostic strings
void Exception_free(Exception *self);

// Human-readable category string
const char *Exception_categoryName(ExceptionCategory category);

// Print Java-style runtime exception diagnostic banner to stderr
void Exception_print(const Exception *self);

#endif // EXCEPTION_EXCEPTION_H
