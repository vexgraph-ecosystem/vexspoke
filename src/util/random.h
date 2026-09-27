#ifndef UTIL_RANDOM_H
#define UTIL_RANDOM_H

#include <stdbool.h>
#include <stdint.h>
#include "c23/constructor.h"

#include "objects/probable.h"
#include "objects/probable_objects.h"

// util/random.h — the Random class, ported from util/Random.java.
//
// Deterministic pseudo-random stream. The stream's algorithm (engine) is
// chosen DETERMINISTICALLY from the seed, so a seed fully determines the whole
// sequence; the caller never has to know which engine ran. Use this for
// gameplay, simulation, and anything that must be reproducible. For secrets,
// tokens, keys, or salts use security/SecureRandom — never this.
//
// One Random block per stream; the system RNG is a lazily-initialized shared
// stream for parameterless draws.

typedef enum RandomEngine {
    RANDOM_ENGINE_MURMUR = 0,   // counter + Murmur3 finalizer (splitmix-family)
    RANDOM_ENGINE_XORSHIFT,     // xorshift64* (13,7,17, then a multiply)
    RANDOM_ENGINE_PCG,          // PCG64 (XSH-RR over a 128-bit LCG state)
    RANDOM_ENGINE_COUNT
} RandomEngine;

typedef struct Random {
    uint64_t seed;        // primary state (engine-dependent)
    uint64_t inc;         // secondary state (PCG low word; 0 otherwise)
    uint32_t counter;     // sequence index (murmur engine)
    RandomEngine engine;  // which algorithm this stream runs
} Random;

// New stream; the engine is chosen deterministically from the seed.
Random *Random_1(uint64_t seed);

// New stream forced onto a specific engine (tests, deterministic reproduction).
Random *Random_2(uint64_t seed, RandomEngine engine);

// Which engine this stream runs (telemetry / tests; gameplay never needs it).
RandomEngine Random_engine(const Random *r);

void Random_free(Random *r);

// Shared system stream, seeded from the monotonic clock on first use.
Random *Random_system(void);

uint64_t Random_nextLong(Random *r);
int32_t Random_nextInt(Random *r);
float Random_nextFloat(Random *r);
double Random_nextDouble(Random *r);
float Random_nextNDCFloat(Random *r);
char Random_nextChar(Random *r);

// True with probability weight/total (legacy getWeight).
bool Random_getWeight(Random *r, uint32_t weight, uint32_t total);

// Roll a Probable: object on a hit, 0 on a miss.
uintptr_t Random_sample(Random *r, const Probable *probable);

// Draw one object from a weighted ProbableObjects pool.
uintptr_t Random_probablePool(Random *r, const ProbableObjects *pool);


#define Random(...) CONSTRUCTOR_DISPATCH(Random, __VA_ARGS__)
#endif