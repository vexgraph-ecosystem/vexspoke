#include "exception.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void Exception_initV(Exception *self,
                     ExceptionCategory category,
                     const char *site,
                     const char *file,
                     int line,
                     const char *fmt,
                     va_list args) {
    if (self == NULL) return;

    (*self).type = TYPE_EXCEPTION_SINGLETON;
    (*self).category = category;
    (*self).site = (site != NULL) ? site : "UnknownSite";
    (*self).file = (file != NULL) ? file : "UnknownFile";
    (*self).line = line;
    (*self).message = NULL;
    (*self).details = NULL;

    if (fmt != NULL) {
        va_list copy;
        va_copy(copy, args);
        int needed = vsnprintf(NULL, 0, fmt, copy);
        va_end(copy);

        if (needed >= 0) {
            char *buf = (char *)malloc((size_t)needed + 1);
            if (buf != NULL) {
                vsnprintf(buf, (size_t)needed + 1, fmt, args);
                (*self).message = buf;
            }
        }
    }
}

void Exception_init(Exception *self,
                    ExceptionCategory category,
                    const char *site,
                    const char *file,
                    int line,
                    const char *fmt,
                    ...) {
    va_list args;
    va_start(args, fmt);
    Exception_initV(self, category, site, file, line, fmt, args);
    va_end(args);
}

void Exception_setDetailsV(Exception *self, const char *fmt, va_list args) {
    if (self == NULL || fmt == NULL) return;

    va_list copy;
    va_copy(copy, args);
    int needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);

    if (needed < 0) return;

    char *buf = (char *)malloc((size_t)needed + 1);
    if (buf != NULL) {
        vsnprintf(buf, (size_t)needed + 1, fmt, args);
        if ((*self).details != NULL) {
            free((*self).details);
        }
        (*self).details = buf;
    }
}

void Exception_setDetails(Exception *self, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Exception_setDetailsV(self, fmt, args);
    va_end(args);
}

void Exception_free(Exception *self) {
    if (self == NULL) return;

    if ((*self).message != NULL) {
        free((*self).message);
        (*self).message = NULL;
    }
    if ((*self).details != NULL) {
        free((*self).details);
        (*self).details = NULL;
    }
}

void Exception_print(const Exception *self) {
    if (self == NULL) return;

    fprintf(stderr, "\n================================================================================\n");
    fprintf(stderr, "RUNTIME EXCEPTION: [%s] at %s\n",
            Exception_categoryName((*self).category),
            (*self).site ? (*self).site : "unknown");
    fprintf(stderr, "Location: %s:%d\n",
            (*self).file ? (*self).file : "unknown",
            (*self).line);
    fprintf(stderr, "--------------------------------------------------------------------------------\n");
    if ((*self).message != NULL && (*self).message[0] != '\0') {
        fprintf(stderr, "Reason / Probable Causes:\n  %s\n", (*self).message);
    }
    if ((*self).details != NULL && (*self).details[0] != '\0') {
        fprintf(stderr, "Variable Dump / Context Diagnostics:\n  %s\n", (*self).details);
    }
    fprintf(stderr, "================================================================================\n\n");
    fflush(stderr);
}
