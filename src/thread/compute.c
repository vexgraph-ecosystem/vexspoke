#include "thread/compute.h"

#include <stdatomic.h>

#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ComputeThread
 * ============================================================================
 * The heavy-work job system: a small fixed pool of supervised Thread workers that
 * run ComputeJob handles. Every role thread shares ONE job shape (a tagged
 * ComputeJob), so a generic job can be submitted to the pool or to any specific
 * established worker (event/draw/ui/scripting/networking) through
 * ComputeThread_submitTo. Dispatch is round-robin: each worker owns its own
 * queue, so the pool balances without a shared ring. Fire-and-forget: a job that
 * needs to publish a result sets a reactive, waking back onto the reactive
 * thread; the reactive thread never blocks here.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: ComputeThread (thread/compute.c)
 * ============================================================================
 * the heavy-work job pool (the parking lot).
 *
 * STRUCT FIELDS (local to this file):
 * ----------------------------------------------------------------------------
 *   Thread *s_pool[COMPUTE_POOL_MAX];  // lazily started workers
 *   size_t s_poolCount;                // live pool size
 *   _Atomic size_t s_rr;               // round-robin cursor
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - ComputeJob_isJob(task) / ComputeJob_run(task)
 *   - ComputeThread_invoke(void)
 *   - ComputeThread_start(count)
 *   - ComputeThread_submit(job) / ComputeThread_submitTo(worker, job)
 *   - ComputeThread_stop(void) / ComputeThread_free(void) / ComputeThread_count(void)
 * ============================================================================
 */

// thread/compute.c — heavy-work job pool.

#define COMPUTE_POOL_MAX 8

static Thread *s_pool[COMPUTE_POOL_MAX];
static size_t s_poolCount = 0;
static _Atomic size_t s_rr = 0;

// The one body every compute worker runs: a popped task is a ComputeJob.
static void compute_job(Thread *self, void *task) {
    (void) self;
    ComputeJob_run(task);                    // ignores non-jobs (nullptr, foreign packets)
}

bool ComputeJob_isJob(const void *task) {
    const ComputeJob *job = (const ComputeJob*) task;
    return job != nullptr && job->tag == COMPUTE_JOB_TAG;
}

bool ComputeJob_run(void *task) {
    ComputeJob *job = (ComputeJob*) task;
    if (!ComputeJob_isJob(job))
        return false;
    if (job->run != nullptr)
        job->run(job->ctx);
    if (job->onDone != nullptr)
        job->onDone(job->ctx);               // fire-and-forget; same worker
    return true;
}

Thread *ComputeThread_invoke(void) {
    Thread *worker = Thread_new(TYPE_THREAD_COMPUTE_SINGLETON, compute_job, 4096, false, false);
    if (worker != nullptr)
        Thread_run(worker);
    return worker;
}

bool ComputeThread_start(size_t count) {
    if (count == 0)
        count = 1;
    if (count > COMPUTE_POOL_MAX)
        count = COMPUTE_POOL_MAX;
    for (size_t i = 0; i < count; i++) {
        if (s_pool[i] != nullptr)
            continue;                        // idempotent: keep the existing worker
        Thread *worker = ComputeThread_invoke();
        if (worker == nullptr)
            return false;
        s_pool[i] = worker;
        if (s_poolCount < i + 1)
            s_poolCount = i + 1;
    }
    return true;
}

bool ComputeThread_submit(ComputeJob *job) {
    if (job == nullptr || s_poolCount == 0)
        return false;
    size_t start = atomic_fetch_add_explicit(&s_rr, 1, memory_order_relaxed);
    for (size_t i = 0; i < s_poolCount; i++) {
        Thread *worker = s_pool[(start + i) % s_poolCount];
        if (worker != nullptr && Thread_submit(worker, job))
            return true;
    }
    return false;
}

bool ComputeThread_submitTo(Thread *worker, ComputeJob *job) {
    return worker != nullptr && job != nullptr && Thread_submit(worker, job);
}

void ComputeThread_stop(void) {
    for (size_t i = 0; i < COMPUTE_POOL_MAX; i++)
        Thread_stop(s_pool[i]);
}

void ComputeThread_free(void) {
    for (size_t i = 0; i < COMPUTE_POOL_MAX; i++) {
        Thread_free(s_pool[i]);
        s_pool[i] = nullptr;
    }
    s_poolCount = 0;
}

size_t ComputeThread_count(void) {
    return s_poolCount;
}
