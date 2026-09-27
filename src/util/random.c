#include "util/random.h"

#include <stdlib.h>
#include <time.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#include "nio/mem.h"
#include "oop/type.h"
#include "util/hash.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Random
 * ============================================================================
 * Deterministic pseudo-random stream with three interchangeable engines
 * (murmur/splitmix, xorshift64*, PCG64). The engine is chosen as a pure
 * function of the seed, so a seed fully determines the sequence and the caller
 * never selects or observes the algorithm. Random_2 forces an engine for tests
 * and reproduction. Random_system lazily seeds a process singleton from the
 * monotonic clock. Weighted sampling draws a uniform 63-bit value and compares
 * against weight/total, or binary-searches a ProbableObjects cumulative column;
 * totalWeight 0 falls back to uniform index selection.
 *
 * For secrets, tokens, or keys use security/SecureRandom — never this class:
 * this one is seedable and therefore predictable by design.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Random (util/random.c)
 * LEVEL: L2 — Behavior (utility behavior API)
 * ============================================================================
 * the Random class, ported from util/Random.java.
 *
 * STRUCT FIELDS (Mirroring util/random.h):
 * ----------------------------------------------------------------------------
 *   Random {
 *     uint64_t seed; // primary state (engine-dependent)
 *     uint64_t inc; // secondary state (PCG low word; 0 otherwise)
 *     uint32_t counter; // sequence index (murmur engine)
 *     RandomEngine engine; // which algorithm this stream runs
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Random_1(seed)
 *   - Random_2(seed, engine)
 *
 * Core Functions:
 *   - Random_free(r)
 *   - Random_system(void)
 *   - Random_nextLong(r)
 *   - Random_nextInt(r)
 *   - Random_nextFloat(r)
 *   - Random_nextDouble(r)
 *   - Random_nextNDCFloat(r)
 *   - Random_nextChar(r)
 *   - Random_sample(r, probable)
 *   - Random_probablePool(r, pool)
 *
 * Getters:
 *   - Random_engine(r)
 *   - Random_getWeight(r, weight, total)
 * ============================================================================
 */


// random.c — Random port (Legacy: util/Random.java).

static const uint64_t GOLDEN_RATIO_64 = 0x9e3779b97f4a7c15ull;

// PCG64 (XSH-RR): a 128-bit LCG multiplier and an odd increment.
static const __uint128_t PCG64_MULT =
    (((__uint128_t) 0x2360ED051FC65DA4ULL) << 64) | (__uint128_t) 0x4385DF649FCCF645ULL;
#define PCG64_INC ((__uint128_t) 1442695040888963407ULL)

static Random *system_rng = nullptr;

static uint64_t systemSeed(void) {
#if defined(__APPLE__)
    uint64_t ticks = (uint64_t) mach_absolute_time();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ticks = (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
#endif
    // Mix (never just shift): a shift would throw the good high bits away.
    // Fold in the stream address so two streams made in the same tick diverge.
    return Hash_murmur3Mix64(ticks) ^ Hash_murmur3Mix64((uint64_t)(uintptr_t) &system_rng);
}

// --- Engines: one step yields one uniform uint64 ---

static uint64_t murmurStep(Random *r) {
    uint64_t mixed = (*r).seed ^ ((uint64_t)(*r).counter * GOLDEN_RATIO_64);
    uint64_t result = Hash_murmur3Mix64(mixed);
    (*r).seed = result;
    (*r).counter++;
    return result;
}

static uint64_t xorshift64StarStep(Random *r) {
    uint64_t x = (*r).seed;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    (*r).seed = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static uint64_t pcg64Step(Random *r) {
    __uint128_t state = ((__uint128_t)(*r).seed << 64) | (__uint128_t)(*r).inc;
    __uint128_t old = state;
    state = old * PCG64_MULT + PCG64_INC;
    uint64_t xored = (uint64_t)((old >> 64) ^ old);
    unsigned rot = (unsigned)(old >> 122);
    (*r).seed = (uint64_t)(state >> 64);
    (*r).inc = (uint64_t) state;
    return (xored >> rot) | (xored << ((64u - rot) & 63u));
}

// The engine choice is a PURE function of the seed: identical seeds must give
// identical streams (the Deterministic Calculation Law forbids dependence on
// time, address, or allocation order).
static RandomEngine pickEngine(uint64_t seed) {
    return (RandomEngine)(Hash_murmur3Mix64(seed) % (uint64_t) RANDOM_ENGINE_COUNT);
}

static void initState(Random *r, uint64_t seed, RandomEngine engine) {
    (*r).engine = engine;
    (*r).counter = 0;
    (*r).inc = 0;
    switch (engine) {
        case RANDOM_ENGINE_XORSHIFT:
            // 0 is xorshift's fixed point; remap it to a nonzero constant.
            (*r).seed = (seed != 0) ? seed : 0x9E3779B97F4A7C15ULL;
            break;
        case RANDOM_ENGINE_PCG:
            (*r).seed = seed;
            (*r).inc = seed;
            break;
        case RANDOM_ENGINE_MURMUR:
        default:
            (*r).seed = seed;
            break;
    }
}

Random *Random_2(uint64_t seed, RandomEngine engine) {
    if ((int) engine < 0 || engine >= RANDOM_ENGINE_COUNT)
        engine = RANDOM_ENGINE_MURMUR;
    Random *r = (Random*) Memory_alloc(TYPE_RANDOM, sizeof(Random));
    if (!r)
        return nullptr;
    initState(r, seed, engine);
    return r;
}

Random *Random_1(uint64_t seed) {
    return Random_2(seed, pickEngine(seed));
}

void Random_free(Random *r) {
    Memory_free(r);
}

RandomEngine Random_engine(const Random *r) {
    return r ? (*r).engine : RANDOM_ENGINE_MURMUR;
}

Random *Random_system(void) {
    if (!system_rng)
        system_rng = Random_1(systemSeed());
    return system_rng;
}

uint64_t Random_nextLong(Random *r) {
    if (!r) return 0;
    switch ((*r).engine) {
        case RANDOM_ENGINE_XORSHIFT: return xorshift64StarStep(r);
        case RANDOM_ENGINE_PCG:      return pcg64Step(r);
        case RANDOM_ENGINE_MURMUR:
        default:                     return murmurStep(r);
    }
}

int32_t Random_nextInt(Random *r) {
    return (int32_t)Random_nextLong(r);
}

float Random_nextFloat(Random *r) {
    return (float)((Random_nextLong(r) & 0xFFFFFF) / 16777216.0);
}

double Random_nextDouble(Random *r) {
    return (double)((Random_nextLong(r) & 0x1FFFFFFFFFFFFFull) / 9007199254740992.0);
}

float Random_nextNDCFloat(Random *r) {
    return (float)(((Random_nextLong(r) & 0xFFFFFF) / 8388608.0) - 1.0);
}

char Random_nextChar(Random *r) {
    int64_t v = (int64_t)Random_nextLong(r);
    return (char)(32 + (llabs(v) % 95));
}

bool Random_getWeight(Random *r, uint32_t weight, uint32_t total) {
    if (!r) return false;
    if (total == 0) return false;
    if (weight >= total) return true;
    if (weight == 0) return false;
    uint64_t val = Random_nextLong(r) & 0x7FFFFFFFFFFFFFFFull;
    return (val % total) < weight;
}

uintptr_t Random_sample(Random *r, const Probable *probable) {
    if (!probable) return 0;
    if (Random_getWeight(r, (*probable).weight, (*probable).total))
        return (*probable).object;
    return 0;
}

uintptr_t Random_probablePool(Random *r, const ProbableObjects *pool) {
    if (!pool) return 0;
    size_t count = ProbableObjects_size((ProbableObjects*) pool);
    if (count == 0) return 0;

    uint32_t total_weight = ProbableObjects_totalWeight((ProbableObjects*) pool);
    uint64_t val = Random_nextLong(r) & 0x7FFFFFFFFFFFFFFFull;

    if (total_weight == 0) {
        size_t idx = (size_t)(val % count);
        return ProbableObjects_objectAt((ProbableObjects*) pool, idx);
    }

    uint32_t target = (uint32_t)(val % total_weight);
    size_t low = 0;
    size_t high = count - 1;
    while (low < high) {
        size_t mid = (low + high) / 2;
        uint32_t cumulative = ProbableObjects_cumulativeAt((ProbableObjects*) pool, mid);
        if (cumulative < target)
            low = mid + 1;
        else
            high = mid;
    }
    return ProbableObjects_objectAt((ProbableObjects*) pool, low);
}
