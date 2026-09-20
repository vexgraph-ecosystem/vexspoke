#include "thread/event.h"

#include "input/key.h"
#include "input/mouse.h"
#include "input/touch.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Event
 * ============================================================================
 * The event dispatcher worker (Legacy: EventThread.java): a dedicated Thread
 * whose job, when no packet is submitted, pumps the input subsystem —
 * dispatching Key, Mouse, and Touch events on the worker thread so the frame
 * loop never blocks on input. EventThread_submit is reserved for off-heap
 * custom event packet dispatch (legacy dispatch()); the current job ignores
 * packets. invoke creates the worker with the TYPE_THREAD_EVENT_SINGLETON
 * type id; stop/free delegate to the Thread lifecycle.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Event (thread/event.c)
 * LEVEL: L2 — Behavior (worker-thread behavior API)
 * ============================================================================
 * the event dispatcher worker (Legacy: EventThread.java).
 *
 * STRUCT FIELDS: none — procedural (operates on Thread pool (event role workers))
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - EventThread_invoke(void)
 *   - EventThread_submit(w, packet)
 *   - EventThread_stop(w)
 *   - EventThread_free(w)
 * ============================================================================
 */


// thread/event.c — event dispatcher worker (Legacy: EventThread.java).

static void event_job(Thread *self, void *task) {
    (void)self;
    if (!task) {
        Key_dispatchEvents();
        Mouse_dispatchEvents();
        Touch_dispatchEvents();
        return;
    }
    // Reserved for off-heap custom event packet dispatchers (legacy dispatch()).
}

Thread *EventThread_invoke(void) {
    return Thread_new(TYPE_THREAD_EVENT_SINGLETON, event_job, 1024, true, false);
}

bool EventThread_submit(Thread *w, void *packet) {
    return Thread_submit(w, packet);
}

void EventThread_stop(Thread *w) {
    Thread_stop(w);
}

void EventThread_free(Thread *w) {
    Thread_free(w);
}
