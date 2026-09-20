#include "cli/console.h"

#include <stdio.h>
#include <stdlib.h>

#include "primitive/string.h"
#include "atomic/ring.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Console
 * ============================================================================
 * The CLI log console (Legacy: cli/Console.java): a process-wide RingBuffer
 * of string blocks that defers printf output until Console_drain, so hot
 * paths never block on the terminal. Exists because R1 hosts and R5 apps need
 * a bounded, allocation-light log path that can be drained on the frame loop.
 * Memory: one static 1024-slot RingBuffer of uint8_t* string handles,
 * initialized once by Console_init. Lifetime: process-scoped;
 * Console_shutdown frees the queue. Adjacent: R2 string primitives feed it;
 * R1 Kernel and R5 apps consume it.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Console (cli/console.c)
 * LEVEL: L2 — Behavior (CLI behavior API)
 * ============================================================================
 * the Console class, ported from cli/Console.java.
 *
 * STRUCT FIELDS: none — procedural (operates on RingBuffer message queue of string blocks)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Console_init(void)
 *
 * Core Functions:
 *   - Console_log(message)
 *   - Console_logString(_string)
 *   - Console_drain(void)
 *   - Console_shutdown(void)
 * ============================================================================
 */


// console.c — the Console class, ported from cli/Console.java.

static RingBuffer *log_queue = nullptr;
static const size_t CONSOLE_QUEUE_CAPACITY = 1024;

void Console_init(void) {
    if (log_queue)
        return;
    RingBuffer *ring = malloc(sizeof(RingBuffer));
    if (!ring)
        return;
    if (!RingBuffer_init(ring, sizeof(uint8_t*), CONSOLE_QUEUE_CAPACITY)) {
        free(ring);
        return;
    }
    log_queue = ring;
}

void Console_logString(uint8_t *_string) {
    if (!_string)
        return;
    if (log_queue) {
        if (RingBuffer_push(log_queue, &_string))
            return;
    }
    printf("%s\n", string_get(_string));
    string_free(_string);
}

void Console_log(const char *message) {
    if (!message)
        return;
    uint8_t *string_ptr = string_allocate(message);
    if (string_ptr)
        Console_logString(string_ptr);
}

void Console_drain(void) {
    if (!log_queue)
        return;
    uint8_t *string_ptr;
    while (RingBuffer_pop(log_queue, &string_ptr)) {
        printf("%s\n", string_get(string_ptr));
        string_free(string_ptr);
    }
}

void Console_shutdown(void) {
    if (!log_queue)
        return;
    Console_drain();
    RingBuffer_shutdown(log_queue);
    free(log_queue);
    log_queue = nullptr;
}
