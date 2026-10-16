#ifndef VEXSPOKE_ALGO_BVH_H
#define VEXSPOKE_ALGO_BVH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BvhAabb {
    float min_point[3];
    float max_point[3];
} BvhAabb;

static inline BvhAabb BvhAabb_0(void) { return (BvhAabb) {0}; }
#define BvhAabb(...) CONSTRUCTOR_DISPATCH(BvhAabb, __VA_ARGS__)
static inline void BvhAabb_setMinimum(BvhAabb *box, float x, float y, float z) {
    if (box) { (*box).min_point[0] = x; (*box).min_point[1] = y; (*box).min_point[2] = z; }
}
static inline void BvhAabb_setMaximum(BvhAabb *box, float x, float y, float z) {
    if (box) { (*box).max_point[0] = x; (*box).max_point[1] = y; (*box).max_point[2] = z; }
}

typedef struct BvhRay {
    float origin[3];
    float direction[3];
    float t_min;
    float t_max;
} BvhRay;

static inline BvhRay BvhRay_8(float originX, float originY, float originZ,
                              float directionX, float directionY, float directionZ,
                              float minimumDistance, float maximumDistance) {
    return (BvhRay) {
        .origin = { originX, originY, originZ },
        .direction = { directionX, directionY, directionZ },
        .t_min = minimumDistance,
        .t_max = maximumDistance
    };
}
#define BvhRay(...) CONSTRUCTOR_DISPATCH(BvhRay, __VA_ARGS__)
static inline void BvhRay_setOrigin(BvhRay *ray, float x, float y, float z) {
    if (ray) { (*ray).origin[0] = x; (*ray).origin[1] = y; (*ray).origin[2] = z; }
}
static inline void BvhRay_setDirection(BvhRay *ray, float x, float y, float z) {
    if (ray) { (*ray).direction[0] = x; (*ray).direction[1] = y; (*ray).direction[2] = z; }
}
static inline void BvhRay_setRange(BvhRay *ray, float minimumDistance, float maximumDistance) {
    if (ray) { (*ray).t_min = minimumDistance; (*ray).t_max = maximumDistance; }
}

typedef struct BvhRayHit {
    bool hit;
    float t;
    uint32_t primitive_id;
    float normal[3];
} BvhRayHit;

static inline BvhRayHit BvhRayHit_0(void) { return (BvhRayHit) {0}; }
#define BvhRayHit(...) CONSTRUCTOR_DISPATCH(BvhRayHit, __VA_ARGS__)

typedef struct BvhPrimitive {
    uint32_t id;
    BvhAabb bounds;
    float centroid[3];
} BvhPrimitive;

static inline BvhPrimitive BvhPrimitive_5(uint32_t id, BvhAabb bounds,
                                          float centroidX, float centroidY, float centroidZ) {
    return (BvhPrimitive) { .id = id, .bounds = bounds,
        .centroid = { centroidX, centroidY, centroidZ } };
}
#define BvhPrimitive(...) CONSTRUCTOR_DISPATCH(BvhPrimitive, __VA_ARGS__)

typedef struct BvhNode {
    BvhAabb bounds;
    int32_t left_child;
    int32_t right_child;
    uint32_t primitive_offset;
    uint32_t primitive_count;
    bool is_leaf;
} BvhNode;

typedef struct BvhTree {
    BvhNode *nodes;
    uint32_t *primitive_indices;
    const BvhPrimitive *primitives;
    uint32_t node_count;
    uint32_t node_capacity;
    uint32_t primitive_count;
} BvhTree;

BvhAabb BvhAabb_empty(void);
BvhAabb BvhAabb_from_points(const float min_pt[3], const float max_pt[3]);
void BvhAabb_expand_point(const float point[3], BvhAabb *box);
void BvhAabb_merge(const BvhAabb *other, BvhAabb *dest);
bool BvhAabb_intersect_ray(const BvhRay *ray, const BvhAabb *box, float *t_near_out);

BvhTree BvhTree_0(void);
BvhTree BvhTree_1(uint32_t primitive_count);
#define BvhTree(...) CONSTRUCTOR_DISPATCH(BvhTree, __VA_ARGS__)
void BvhTree_destroy(BvhTree *tree);
bool BvhTree_build(const BvhPrimitive *prims, uint32_t count, BvhTree *tree);
bool BvhTree_intersect_ray(const BvhTree *tree, const BvhRay *ray, BvhRayHit *hit_out);

#ifdef __cplusplus
}
#endif

#endif // VEXSPOKE_ALGO_BVH_H
