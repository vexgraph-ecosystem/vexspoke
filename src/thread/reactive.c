#include "thread/reactive.h"

#include <stdint.h>

#include "atomic/spin.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "struct/map.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveThread
 * ============================================================================
 * The reactive delivery worker: one supervised Thread whose only job is to drain
 * queued reactive boxes on its own platform thread. A producer on any thread
 * stores a value and calls ReactiveThread_wake(box); the queued set (one entry
 * per box, under a spinlock) guarantees the box is enqueued once, so many stores
 * collapse into one drain. The job pops a box, clears its queued entry, and calls
 * Reactive_drain — so the box's onSet/onChanged/onNullptr observers run on this
 * thread, coalesced, in FIFO order.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: ReactiveThread (thread/reactive.c)
 * ============================================================================
 * the reactive delivery worker (the generic R2 owner).
 *
 * STRUCT FIELDS (local to this file):
 * ----------------------------------------------------------------------------
 *   Thread *s_core;          // shared default worker (lazy singleton)
 *   SpinLock s_lock;         // guards s_queued
 *   Map *s_queued;           // box pointer => 1 (queued, not yet drained)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - ReactiveThread_invoke(void)
 *   - ReactiveThread_core(void) / ReactiveThread_start(void) / ReactiveThread_shutdown(void)
 *   - ReactiveThread_wake(box) / ReactiveThread_wakeOn(worker, box)
 *   - ReactiveThread_adopt(box) / ReactiveThread_release(box)
 *   - ReactiveThread_isQueued(box) / ReactiveThread_pending(void)
 *   - ReactiveThread_stop(worker) / ReactiveThread_free(worker)
 * ============================================================================
 */

// thread/reactive.c — reactive delivery worker.

static Thread *s_core = nullptr;
static SpinLock s_lock = SPIN_LOCK_INIT;
static Map *s_queued = nullptr;

static void deliverToReactiveThread(Reactive *box); // the default delivery hook

static Map *queued(void) {
    if (s_queued == nullptr)
        s_queued = Map_3(ID_LONG, ID_LONG, 64);
    return s_queued;
}

// One pass: pop a box, forget it is queued, drain it. Dropping the queued entry
// BEFORE the drain lets a callback that re-wakes the same box queue it for the
// NEXT pass (coalesced, and never a same-stack re-entry).
static void reactive_job(Thread *self, void *task) {
    (void) self;
    if (task == nullptr)
        return;                              // no idle work: this worker only acts when nudged
    Reactive *box = (Reactive*) task;
    SpinLock_lock(&s_lock);
    Map *q = queued();
    if (q != nullptr)
        Map_remove(q, (uint64_t)(uintptr_t) box);
    SpinLock_unlock(&s_lock);
    Reactive_drain(box);                     // observers run HERE, on the reactive thread
}

Thread *ReactiveThread_invoke(void) {
    return Thread_new(TYPE_THREAD_REACTIVE_SINGLETON, reactive_job, 2048, false, false);
}

Thread *ReactiveThread_core(void) {
    if (s_core == nullptr) {
        s_core = Thread_new(TYPE_THREAD_REACTIVE_SINGLETON, reactive_job, 2048, false, false);
        if (s_core != nullptr)
            Thread_run(s_core);              // the default owner is live on first use
    }
    return s_core;
}

// Adopt the shared worker as the process default delivery: from here, a plain
// set on any box that has not opted out schedules this worker's drain.
bool ReactiveThread_start(void) {
    Thread *core = ReactiveThread_core();
    if (core == nullptr)
        return false;
    Reactive_setDefaultDelivery(deliverToReactiveThread);
    return true;
}

void ReactiveThread_shutdown(void) {
    if (Reactive_getDefaultDelivery() == deliverToReactiveThread)
        Reactive_setDefaultDelivery(nullptr);
    if (s_core != nullptr) {
        Thread *core = s_core;
        s_core = nullptr;
        Thread_stop(core);
        Thread_free(core);
    }
}

// Queue-once: the win is deciding "is it already queued?" under the lock, so N
// concurrent wakes still submit at most one box.
static bool queueBox(Thread *worker, Reactive *box) {
    if (worker == nullptr || box == nullptr)
        return false;
    uint64_t key = (uint64_t)(uintptr_t) box;
    SpinLock_lock(&s_lock);
    Map *q = queued();
    bool already = (q != nullptr) && Map_containsKey(q, key);
    if (!already && q != nullptr)
        Map_put(q, key, 1);
    SpinLock_unlock(&s_lock);
    if (already)
        return true;                         // one queue, NOT one per store
    if (!Thread_submit(worker, box)) {
        SpinLock_lock(&s_lock);
        Map_remove(queued(), key);           // roll back so a later wake retries
        SpinLock_unlock(&s_lock);
        return false;
    }
    return true;
}

bool ReactiveThread_wakeOn(Thread *worker, Reactive *box) {
    return queueBox(worker, box);
}

bool ReactiveThread_wake(Reactive *box) {
    return queueBox(ReactiveThread_core(), box);
}

// The delivery hook: bound to a box so a plain set schedules this worker. It
// runs on the WRITER's thread and only queues — never fires observers.
static void deliverToReactiveThread(Reactive *box) {
    ReactiveThread_wake(box);
}

bool ReactiveThread_adopt(Reactive *box) {
    if (box == nullptr)
        return false;
    Reactive_setDelivery(box, deliverToReactiveThread);
    return true;
}

void ReactiveThread_release(Reactive *box) {
    if (box == nullptr)
        return;
    Reactive_setDelivery(box, nullptr);
    SpinLock_lock(&s_lock);
    if (s_queued != nullptr)
        Map_remove(s_queued, (uint64_t)(uintptr_t) box);
    SpinLock_unlock(&s_lock);
}

bool ReactiveThread_isQueued(const Reactive *box) {
    if (box == nullptr)
        return false;
    SpinLock_lock(&s_lock);
    bool q = (s_queued != nullptr) && Map_containsKey(s_queued, (uint64_t)(uintptr_t) box);
    SpinLock_unlock(&s_lock);
    return q;
}

size_t ReactiveThread_pending(void) {
    SpinLock_lock(&s_lock);
    size_t n = (s_queued != nullptr) ? Map_size(s_queued) : 0;
    SpinLock_unlock(&s_lock);
    return n;
}

void ReactiveThread_stop(Thread *worker) {
    Thread_stop(worker);
}

void ReactiveThread_free(Thread *worker) {
    if (worker == s_core)
        return;                              // the default owner outlives its callers
    Thread_free(worker);
}
