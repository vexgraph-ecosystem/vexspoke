#include "exception.h"
#include "annotation/definition.h"
#include "annotation/overview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

;;DEFINITION
/** Exception owns formatted diagnostic strings and source metadata for a cold
 * failure report. Formatting and printing may allocate or block, so this is a
 * diagnostic/lifecycle object rather than a real-time error channel. */
;;OVERVIEW
/** ========================================================================
 * CLASS: Exception (exception/exception.c)
 * STRUCT FIELDS (Mirroring exception.h)
 * uint64_t type                 project-scoped Exception singleton identity
 * ExceptionCategory category    stable diagnostic category
 * const char *site              borrowed operation/site label
 * const char *file              borrowed source filename
 * int line                      source line number
 * char *message                 owned formatted primary diagnostic
 * char *details                 owned formatted context diagnostic
 * FUNCTION REGISTRY
 * Projections: const char *Exception_categoryName(ExceptionCategory category)
 * Constructors: Exception_0(), Exception_5(..), Exception_6(..),
 *   Exception_7(..), Exception_8(..)
 * Lifecycle: void Exception_free(Exception *self)
 * Setters: void Exception_setDetails(Exception *self, const char *fmt, ...)
 *   void Exception_setDetailsV(Exception *self, const char *fmt, va_list args)
 * Operations: void Exception_print(const Exception *self)
 * Private behavior: format sizing/copying is performed by exceptionInitializeV
 *   and setDetailsV;
 *   allocation failure preserves the prior details value on replacement.
 * Null/default, failure and ownership contract:
 * Exception_0 produces a valid empty diagnostic. Null self is ignored by
 * setters, free and print. Formatted message/details allocations are owned by
 * Exception and released by free;
 * site/file strings are borrowed. stderr formatting is cold and synchronous.
 * ======================================================================== */

// Returns the stable display name for a category, or the unknown-category name.
const char *Exception_categoryName(ExceptionCategory category) {
    switch (category) {
        case EXCEPTION_RUNTIME:        return "RuntimeException";
        case EXCEPTION_WINDOW:         return "WindowException";
        case EXCEPTION_GRAPHICS:       return "GraphicsException";
        case EXCEPTION_NULL_POINTER:   return "NullPointerException";
        case EXCEPTION_ILLEGAL_STATE:  return "IllegalStateException";
        case EXCEPTION_IO:             return "IOException";
        case EXCEPTION_OUT_OF_MEMORY:  return "OutOfMemoryException";
        default:                       return "UnknownException";
    }
}

// Initializes exception metadata and formats an optional message using args.
static void exceptionInitializeV(Exception *self,
                                 ExceptionCategory category,
                                 const char *site,
                                 const char *file,
                                 int line,
                                 const char *fmt,
                                 va_list args) {
    (*self).type = TYPE_EXCEPTION_SINGLETON;
    (*self).category = category;
    (*self).site = (site != nullptr) ? site : "UnknownSite";
    (*self).file = (file != nullptr) ? file : "UnknownFile";
    (*self).line = line;
    (*self).message = nullptr;
    (*self).details = nullptr;

    if (fmt != nullptr) {
        va_list copy;
        va_copy(copy, args);
        int needed = vsnprintf(nullptr, 0, fmt, copy);
        va_end(copy);

        if (needed >= 0) {
            char *buf = (char*) malloc((size_t)needed + 1);
            if (buf != nullptr) {
                vsnprintf(buf, (size_t)needed + 1, fmt, args);
                (*self).message = buf;
            }
        }
    }
}

Exception Exception_0(void) {
    Exception self;
    memset(&self, 0, sizeof self);
    self.type = TYPE_EXCEPTION_SINGLETON;
    self.category = EXCEPTION_RUNTIME;
    self.site = "UnknownSite";
    self.file = "UnknownFile";
    return self;
}

static Exception exceptionFormatted(ExceptionCategory category,
                                     const char *site,
                                     const char *file,
                                     int line,
                                     const char *fmt,
                                     va_list args) {
    Exception self = Exception_0();
    exceptionInitializeV(&self, category, site, file, line, fmt, args);
    return self;
}

Exception Exception_5(ExceptionCategory category, const char *site,
                      const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Exception self = exceptionFormatted(category, site, file, line, fmt, args);
    va_end(args);
    return self;
}

Exception Exception_6(ExceptionCategory category, const char *site,
                      const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Exception self = exceptionFormatted(category, site, file, line, fmt, args);
    va_end(args);
    return self;
}

Exception Exception_7(ExceptionCategory category, const char *site,
                      const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Exception self = exceptionFormatted(category, site, file, line, fmt, args);
    va_end(args);
    return self;
}

Exception Exception_8(ExceptionCategory category, const char *site,
                      const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Exception self = exceptionFormatted(category, site, file, line, fmt, args);
    va_end(args);
    return self;
}

// Replaces owned details with formatted args when formatting and allocation succeed.
void Exception_setDetailsV(Exception *self, const char *fmt, va_list args) {
    if (self == nullptr || fmt == nullptr) return;

    va_list copy;
    va_copy(copy, args);
    int needed = vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);

    if (needed < 0) return;

    char *buf = (char*) malloc((size_t)needed + 1);
    if (buf != nullptr) {
        vsnprintf(buf, (size_t)needed + 1, fmt, args);
        if ((*self).details != nullptr) {
            free((*self).details);
        }
        (*self).details = buf;
    }
}

// Replaces owned details with formatted variadic text.
void Exception_setDetails(Exception *self, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Exception_setDetailsV(self, fmt, args);
    va_end(args);
}

// Releases the owned message and details strings and clears their pointers.
void Exception_free(Exception *self) {
    if (self == nullptr) return;

    if ((*self).message != nullptr) {
        free((*self).message);
        (*self).message = nullptr;
    }
    if ((*self).details != nullptr) {
        free((*self).details);
        (*self).details = nullptr;
    }
}

// Writes category, source location, message, and details to stderr.
void Exception_print(const Exception *self) {
    if (self == nullptr) return;

    fprintf(stderr, "\n================================================================================\n");
    fprintf(stderr, "RUNTIME EXCEPTION: [%s] at %s\n",
            Exception_categoryName((*self).category),
            (*self).site ? (*self).site : "unknown");
    fprintf(stderr, "Location: %s:%d\n",
            (*self).file ? (*self).file : "unknown",
            (*self).line);
    fprintf(stderr, "--------------------------------------------------------------------------------\n");
    if ((*self).message != nullptr && (*self).message[0] != '\0') {
        fprintf(stderr, "Reason / Probable Causes:\n  %s\n", (*self).message);
    }
    if ((*self).details != nullptr && (*self).details[0] != '\0') {
        fprintf(stderr, "Variable Dump / Context Diagnostics:\n  %s\n", (*self).details);
    }
    fprintf(stderr, "================================================================================\n\n");
    fflush(stderr);
}
