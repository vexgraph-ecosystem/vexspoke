#ifndef THREAD_COMPUTE_H
#define THREAD_COMPUTE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "thread/thread.h"

// thread/compute.h — the heavy-work job system (the parking lot).
//
// A ComputeJob is a self-describing task: run(ctx) does the compute, then the
// optional onDone(ctx) runs on the same worker. The job carries COMPUTE_JOB_TAG
// so ANY established thread can recognize and run one it is handed — the one-line
// convention that turns every role (event/draw/ui/scripting/networking) into a
// possible compute thread without teaching it what the job means.
//
// The reactive thread hands heavy or blocking work here and RETURNS. A job that
// must publish a result sets a reactive, which wakes back onto the reactive
// thread. Never submit-and-wait: the reactive thread must not block.

#define COMPUTE_JOB_TAG 0x434F4D50554A4F42ull   // "COMPUJOB"

typedef void (*ComputeJobFn)(void *ctx);

typedef struct ComputeJob {
    uint64_t tag;        // COMPUTE_JOB_TAG — recognition for any role thread
    ComputeJobFn run;    // the heavy/blocking body (runs on a worker)
    void *ctx;           // its argument (caller-owned; keep alive until run)
    ComputeJobFn onDone; // optional completion (runs on the same worker)
} ComputeJob;

// Recognise / execute a task as a ComputeJob. A role's job calls isJob first;
// when true it runs the job and returns, so generic work may be dispatched to
// any established thread (the one-line convention per role).
bool ComputeJob_isJob(const void *task);
bool ComputeJob_run(void *task);             // true when handled as a job

// A private compute worker (caller owns the handle), created running.
Thread *ComputeThread_invoke(void);

// The shared pool: lazily starts `count` workers (idempotent; clamped 1..8).
bool ComputeThread_start(size_t count);
// Round-robin submit into the pool. False when the pool has no free worker.
bool ComputeThread_submit(ComputeJob *job);
// Submit to a specific established worker (any role).
bool ComputeThread_submitTo(Thread *worker, ComputeJob *job);

void ComputeThread_stop(void);
void ComputeThread_free(void);
size_t ComputeThread_count(void);

#endif // THREAD_COMPUTE_H
