#include "thread/scripting.h"

#include "annotation/incomplete.h"
#include "annotation/definition.h"

#include "oop/type.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Scripting
 * ============================================================================
 * The scripting worker (Legacy: ScriptingThread.java). Script executor port:
 * tasks are dequeued and dropped until the scripting surface lands, while the
 * queue and lifecycle are fully live. ScriptingThread_invoke creates a
 * scripting-role Thread whose job currently discards tasks; submit/stop/free
 * delegate straight to the Thread class.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Scripting (thread/scripting.c)
 * LEVEL: L2 — Behavior (worker-thread behavior API)
 * ============================================================================
 * the scripting worker (Legacy: ScriptingThread.java).
 *
 * STRUCT FIELDS: none — procedural (operates on Thread pool (scripting role workers))
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - ScriptingThread_invoke(void)
 *   - ScriptingThread_submit(w, task)
 *   - ScriptingThread_stop(w)
 *   - ScriptingThread_free(w)
 * ============================================================================
 */


// thread/scripting.c — scripting worker (Legacy: ScriptingThread.java).

;;INCOMPLETE // Script executor port: tasks are dequeued and dropped until the
;;INCOMPLETE // scripting surface lands. Queue and lifecycle are fully live.

static void scripting_job(Thread *self, void *task) {
    (void)self;
    (void)task; // TODO(lang): execute(task) once the script surface lands.
}

Thread *ScriptingThread_invoke(void) {
    return Thread_new(TYPE_THREAD_SCRIPTING_SINGLETON, scripting_job,
                      1024, false, false);
}

bool ScriptingThread_submit(Thread *w, void *task) {
    return Thread_submit(w, task);
}

void ScriptingThread_stop(Thread *w) {
    Thread_stop(w);
}

void ScriptingThread_free(Thread *w) {
    Thread_free(w);
}
