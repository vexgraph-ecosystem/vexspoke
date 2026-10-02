#ifndef DISPATCH_DISPATCH_H
#define DISPATCH_DISPATCH_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include "atomic/spin.h"

// MPSC mailbox: producers post; one designated consumer drains on its own
// thread (the UI host chooses Thread 0). No worker, wakeup or scheduler here.
// Context is borrowed, opaque and never inspected. Keep context AND callback
// code alive until delivery or externally coordinated shutdown.
//
// One drain snapshots a FIFO batch. Callback posts wait for a later pass.
// Nested/concurrent drains return false. Callbacks run outside the post lock
// and must return normally and promptly; a whole batch has no time budget.
// Reusable double buffers avoid allocation during drain. Post may grow its
// pending buffer at a new high-water mark: not a hard realtime operation.
// Reserve/growth budgeting and typed Signal coalescing are separate concerns.
typedef void (*DispatchFn)(void *context);

// Behaviorless slot record, storage owned exclusively by Dispatch.
typedef struct DispatchItem {
    DispatchFn fn;
    void *context;
} DispatchItem;

typedef struct Dispatch {
    SpinLock lock;                    // one-shot post/snapshot admission
    DispatchItem *items;              // producer buffer, guarded by lock
    size_t count;
    size_t cap;
    DispatchItem *batch;              // consumer buffer, guarded by draining
    size_t batchCap;
    atomic_bool draining;
    atomic_size_t pendingCount;       // pending only, excludes active batch
    _Atomic unsigned long drainedCount;
} Dispatch;

Dispatch *Dispatch_0(void);
#define Dispatch() Dispatch_0()

// Stop producers and exclude other callers BEFORE free; also never free from
// a handler. Returns false on null or active drain. Dropped jobs are not invoked
// or disposed: the owner must arrange cleanup of borrowed contexts itself.
bool Dispatch_free(Dispatch *self);

// True transfers responsibility for delivery to the queue, not payload ownership.
// False = invalid, contention, capacity overflow or OOM; nothing was enqueued.
// Caller retains responsibility and may retry LATER, never spin on Thread 0.
bool Dispatch_post(Dispatch *self, DispatchFn fn, void *context);

// True = captured batch processed (including empty); false = null or busy.
// No allocation, no waiting for the post lock, no callback under that lock.
bool Dispatch_drain(Dispatch *self);

// Atomic telemetry snapshots; not lifetime or shutdown synchronization.
size_t Dispatch_pending(const Dispatch *self);
unsigned long Dispatch_getDrainedCount(const Dispatch *self);

#endif
