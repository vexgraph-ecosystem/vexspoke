#include "cli/console.h"
#include "primitive/string.h"
#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"
#include <stdio.h>

;;DEFINITION
/** Bare R2 cold one-line output, not a terminal application or queue. No screen,
 * cursor, input, worker or R3 dependency is owned. System's existing init call
 * remains harmless. Output uses synchronous stdio and is not realtime/bounded. */
;;OVERVIEW
/** ========================================================================
 * MODULE: Console (cli/console.c)
 * ------------------------------------------------------------------------
 * STATE: none; no queue, allocation or retained message.
 * FUNCTION REGISTRY (exports)
 * Lifecycle: void Console_init(void), Console_drain(void), Console_shutdown(void)
 *   Compatibility no-ops; never acquire/clear a terminal.
 * Output: void Console_log(const char *message)
 *   Borrowed NUL-terminated text, controls escaped, one final newline.
 * Output: void Console_logString(uint8_t *message)
 *   Takes ownership of a live R2 string block; releases it after output.
 * PRIVATE BEHAVIOR: log writes printable bytes; escapes controls as \\xHH.
 * CONTRACT: cold caller-thread stdio; null rejects once with THROW. A write
 * failure reports once and may have written a prefix (no rollback of stdio).
 * Managed screens must route output through their presentation seam instead.
 * ======================================================================== */
void Console_init(void) {}
void Console_drain(void) {}
void Console_shutdown(void) {}

void Console_log(const char *message) {
    if (message == nullptr) {
        THROW("Console log: null message");
        return;
    }
    bool ok = true;
    for (const unsigned char *p = (const unsigned char*) message; *p != 0; ++p) {
        unsigned char ch = *p;
        if (ch < 32 || ch == 127) {
            if (fprintf(stdout, "\\x%02X", ch) < 0)
                ok = false;
        } else if (fputc(ch, stdout) == EOF)
            ok = false;
        if (!ok)
            break;
    }
    if (ok)
        ok = fputc('\n', stdout) != EOF;
    if (!ok)
        THROW("Console log: output failed");
}

void Console_logString(uint8_t *message) {
    Console_log(string_get(message));
    string_free(message);
}
