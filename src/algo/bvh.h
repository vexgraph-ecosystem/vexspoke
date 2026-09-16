#ifndef VEXSPOKE_ALGO_BVH_H
#define VEXSPOKE_ALGO_BVH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BvhAabb {
    float min_point[3];
    float max_point[3];
} BvhAabb;

typedef struct BvhRay {
    float origin[3];
    float direction[3];
    float t_min;
    float t_max;
} BvhRay;

typedef struct BvhRayHit {
    bool hit;
    float t;
    uint32_t primitive_id;
    float normal[3];
} BvhRayHit;

typedef struct BvhPrimitive {
    uint32_t id;
    BvhAabb bounds;
    float centroid[3];
} BvhPrimitive;

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

bool BvhTree_init(uint32_t primitive_count, BvhTree *tree);
void BvhTree_destroy(BvhTree *tree);
bool BvhTree_build(const BvhPrimitive *prims, uint32_t count, BvhTree *tree);
bool BvhTree_intersect_ray(const BvhTree *tree, const BvhRay *ray, BvhRayHit *hit_out);

#ifdef __cplusplus
}
#endif

#endif // VEXSPOKE_ALGO_BVH_H
