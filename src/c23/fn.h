#ifndef C23_FN_H
#define C23_FN_H

#include <stdint.h>

#include "c23/constructor.h"

// c23/fn.h — functions as data (the opcode-in-hand class).
//
//   int base = 10;
//   Function f = Function(addTen_impl, &base);
//   int *out = Function_run(&f, &x);              // like Java's runnable.run()
//
// A Function is pure data: one code pointer + one environment pointer — the
// same shape as Loop.tick/userdata, promoted to a reusable class. Capture is
// manual (pack what you need into context), which keeps it zero-allocation.
//
// There is exactly ONE runner verb. Function_run takes a single void* because
// in a everything-is-a-pointer engine, "an array of objects" IS one pointer —
// pass a stack array, an Array* block, or a Fields-struct. No varargs: `...`
// would trade away every type check this package exists to keep.
//
// Opcodes in practice: store Functions in arrays/maps and run them later —
// command buffers, listener lists, job queues. Thread_Job bodies dispatching
// through Function_run turn any worker into an executor.

typedef struct Function {
    void *(*call)(void *context, void *in);
    void *context;
} Function;

typedef void *(*FunctionCallFn)(void *context, void *in);

static inline Function Function_0(void) {
    Function function;
    function.call = nullptr;
    function.context = nullptr;
    return function;
}

static inline Function Function_1(FunctionCallFn call) {
    Function function = Function_0();
    function.call = call;
    return function;
}

static inline Function Function_2(FunctionCallFn call, void *context) {
    Function function = Function_1(call);
    function.context = context;
    return function;
}

#define Function(...) CONSTRUCTOR_DISPATCH(Function, __VA_ARGS__)

static inline bool Function_setCall(Function *function, FunctionCallFn call) {
    if (!function) return false;
    (*function).call = call;
    return true;
}

static inline bool Function_setContext(Function *function, void *context) {
    if (!function) return false;
    (*function).context = context;
    return true;
}

static inline FunctionCallFn Function_getCall(const Function *function) {
    return function ? (*function).call : nullptr;
}

static inline void *Function_getContext(const Function *function) {
    return function ? (*function).context : nullptr;
}

// Invoke the function with its captured environment. Returns its result, or
// nullptr when the function has no callback.
static inline void *Function_run(Function *function, void *input) {
    FunctionCallFn call = Function_getCall(function);
    return call ? call(Function_getContext(function), input) : nullptr;
}

// Bind the same code to a fresh environment. The source remains unchanged.
static inline Function Function_bind(const Function *function, void *context) {
    FunctionCallFn call = Function_getCall(function);
    return call ? Function_2(call, context) : Function_0();
}

#endif
