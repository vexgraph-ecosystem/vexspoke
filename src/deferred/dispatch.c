#include "deferred/dispatch.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "annotation/overview.h"

;;OVERVIEW
/**
 * CLASS: Dispatch — L2 Behavior, MPSC mailbox with one consumer.
 * FIELDS: SpinLock lock; DispatchItem *items; size_t count, cap;
 * DispatchItem *batch; size_t batchCap; atomic_bool draining;
 * atomic_size_t pendingCount; _Atomic unsigned long drainedCount.
 * SLOT: DispatchItem { DispatchFn fn; void *context; } (behaviorless).
 * CONSTRUCTOR: Dispatch() / Dispatch_0().
 * CORE: Dispatch_drain, Dispatch_free. SETTER: Dispatch_post.
 * GETTERS: Dispatch_pending, Dispatch_getDrainedCount. HELPERS: none.
 *
 * Single-try admission never spins. Pending storage grows with checked
 * doubling. Drain swaps reusable buffers under the lock, then invokes its
 * snapshot outside it. Posts made during delivery remain pending next pass.
 * Atomic draining rejects nested/concurrent drains; telemetry is read-only.
 * Payloads are borrowed and opaque. No scheduling, coalescing or ownership
 * transfer of context. Free requires external exclusion of all API callers.
 */
Dispatch *Dispatch_0(void) {
    Dispatch *self = (Dispatch*) calloc(1, sizeof(Dispatch));
    if (!self)
        return nullptr;
    SpinLock *lock = &(*self).lock;
    atomic_init(&(*lock).word, 0);
    atomic_init(&(*self).draining, false);
    atomic_init(&(*self).pendingCount, 0);
    atomic_init(&(*self).drainedCount, 0);
    return self;
}

bool Dispatch_post(Dispatch *self, DispatchFn fn, void *context) {
    if (!self || !fn)
        return false;
    if (!SpinLock_tryLock(&(*self).lock))
        return false;
    if ((*self).count == (*self).cap) {
        if ((*self).cap > SIZE_MAX / sizeof(DispatchItem) / 2) {
            SpinLock_unlock(&(*self).lock);
            return false;
        }
        size_t cap = (*self).cap ? (*self).cap * 2 : 8;
        DispatchItem *items = (DispatchItem*) realloc((*self).items, cap * sizeof(DispatchItem));
        if (!items) {
            SpinLock_unlock(&(*self).lock);
            return false;
        }
        (*self).items = items;
        (*self).cap = cap;
    }
    DispatchItem *item = &(*self).items[(*self).count];
    (*item).fn = fn;
    (*item).context = context;
    (*self).count++;
    atomic_store_explicit(&(*self).pendingCount, (*self).count, memory_order_relaxed);
    SpinLock_unlock(&(*self).lock);
    return true;
}

bool Dispatch_drain(Dispatch *self) {
    if (!self)
        return false;
    bool expected = false;
    if (!atomic_compare_exchange_strong_explicit(&(*self).draining, &expected, true,
                                                 memory_order_acquire, memory_order_relaxed))
        return false;
    if (!SpinLock_tryLock(&(*self).lock)) {
        atomic_store_explicit(&(*self).draining, false, memory_order_release);
        return false;
    }
    size_t n = (*self).count;
    if (n != 0) {
        DispatchItem *batch = (*self).items;
        size_t cap = (*self).cap;
        (*self).items = (*self).batch;
        (*self).cap = (*self).batchCap;
        (*self).batch = batch;
        (*self).batchCap = cap;
        (*self).count = 0;
        atomic_store_explicit(&(*self).pendingCount, 0, memory_order_relaxed);
    }
    SpinLock_unlock(&(*self).lock);
    for (size_t i = 0; i < n; i++) {
        DispatchItem *item = &(*self).batch[i];
        (*item).fn((*item).context);
        atomic_fetch_add_explicit(&(*self).drainedCount, 1, memory_order_relaxed);
    }
    atomic_store_explicit(&(*self).draining, false, memory_order_release);
    return true;
}

bool Dispatch_free(Dispatch *self) {
    if (!self)
        return false;
    if (atomic_load_explicit(&(*self).draining, memory_order_acquire))
        return false;
    if ((*self).count > 0)
        fprintf(stderr, "dispatch: dropping %zu undelivered item(s) at free\n", (*self).count);
    free((*self).items);
    free((*self).batch);
    free(self);
    return true;
}

size_t Dispatch_pending(const Dispatch *self) {
    return self ? atomic_load_explicit(&(*self).pendingCount, memory_order_relaxed) : 0;
}

unsigned long Dispatch_getDrainedCount(const Dispatch *self) {
    return self ? atomic_load_explicit(&(*self).drainedCount, memory_order_relaxed) : 0;
}
