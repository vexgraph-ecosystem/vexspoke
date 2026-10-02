#ifndef THREAD_REACTIVE_H
#define THREAD_REACTIVE_H

#include <stdbool.h>
#include <stddef.h>

#include "thread/thread.h"
#include "reactive/reactive.h"

// thread/reactive.h — the reactive delivery worker (the R2 default owner).
//
// One worker drains reactive boxes ON ITS thread. A producer (any thread) calls
// Reactive_store(box, ...) then ReactiveThread_wake(box); the worker pops the box
// and calls Reactive_drain(box), so the box's observers run on the reactive
// thread. A box is queued AT MOST ONCE (the queued set), so N stores between two
// drains cost ONE callback; a callback that wakes another box queues it for the
// NEXT pass — the natural cycle guard.
//
// This is the generic, non-UI owner: it knows nothing about darling or graphvex.
// A box whose observer must run on the UI thread is woken through the UI owner
// instead; a box that needs heavy work hands a job to thread/compute.h and wakes
// nothing until the job publishes a result.

// A private delivery worker (caller owns the handle).
Thread *ReactiveThread_invoke(void);
// The shared default worker (lazy, created running). wake() submits here.
Thread *ReactiveThread_core(void);

// Boot the default owner: start the shared worker and bind it as the PROCESS
// default delivery, so a plain set on ANY box (that has not opted out) schedules
// its drain here — set is the only verb. Call once at startup; idempotent.
bool ReactiveThread_start(void);
// Unbind the default delivery and tear the shared worker down.
void ReactiveThread_shutdown(void);

// Queue a box for owner-thread drain. Safe from any thread. True when the box is
// queued (or already was). false only when the worker is stopped or the queue is
// full. The box must stay alive until it has been drained.
bool ReactiveThread_wake(Reactive *box);
bool ReactiveThread_wakeOn(Thread *worker, Reactive *box);

// Opt a box into the reactive thread: a plain set on `box` (from any thread)
// then schedules its drain here, so the observers run on the reactive thread
// with no Reactive_drain call at the callsite. This is the "set is the default"
// wiring — bind once at construction, then only ever set/get.
bool ReactiveThread_adopt(Reactive *box);
// Unbind a previously adopted box (and drop any pending wake).
void ReactiveThread_release(Reactive *box);

// True while the box is waiting in the queue (not yet drained).
bool ReactiveThread_isQueued(const Reactive *box);
// Boxes waiting in the shared queue.
size_t ReactiveThread_pending(void);

void ReactiveThread_stop(Thread *worker);
void ReactiveThread_free(Thread *worker);   // refuses the core

#endif // THREAD_REACTIVE_H
