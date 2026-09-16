#include "algo/bvh.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "annotation/overview.h"
#include "annotation/intention.h"

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Bvh (algo/bvh.c)
 * LEVEL: L3 — Structural Subsystem (Spatial Acceleration)
 * ============================================================================
 * Bounding Volume Hierarchy (BVH) for accelerated 3D ray tracing and spatial queries.
 *
 * STRUCT FIELDS:
 *   - BvhAabb: min_point[3], max_point[3]
 *   - BvhRay: origin[3], direction[3], t_min, t_max
 *   - BvhTree: nodes, primitive_indices, primitives, counts
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * AABB Utilities:
 *   - BvhAabb_empty()
 *   - BvhAabb_from_points(min_pt, max_pt)
 *   - BvhAabb_expand_point(point, box)
 *   - BvhAabb_merge(other, dest)
 *   - BvhAabb_intersect_ray(ray, box, t_near_out)
 * Tree Operations:
 *   - BvhTree_init(primitive_count, tree)
 *   - BvhTree_destroy(tree)
 *   - BvhTree_build(prims, count, tree)
 *   - BvhTree_intersect_ray(tree, ray, hit_out)
 * ============================================================================
 */

;;INTENTION("Accelerated spatial ray tracing and AABB bounding volume queries")

static inline float fminf_local(float a, float b) {
    return (a < b) ? a : b;
}

static inline float fmaxf_local(float a, float b) {
    return (a > b) ? a : b;
}

BvhAabb BvhAabb_empty(void) {
    BvhAabb box;
    box.min_point[0] = FLT_MAX;
    box.min_point[1] = FLT_MAX;
    box.min_point[2] = FLT_MAX;
    box.max_point[0] = -FLT_MAX;
    box.max_point[1] = -FLT_MAX;
    box.max_point[2] = -FLT_MAX;
    return box;
}

BvhAabb BvhAabb_from_points(const float min_pt[3], const float max_pt[3]) {
    BvhAabb box;
    box.min_point[0] = min_pt[0];
    box.min_point[1] = min_pt[1];
    box.min_point[2] = min_pt[2];
    box.max_point[0] = max_pt[0];
    box.max_point[1] = max_pt[1];
    box.max_point[2] = max_pt[2];
    return box;
}

void BvhAabb_expand_point(const float point[3], BvhAabb *box) {
    if (!point || !box) {
        return;
    }
    (*box).min_point[0] = fminf_local((*box).min_point[0], point[0]);
    (*box).min_point[1] = fminf_local((*box).min_point[1], point[1]);
    (*box).min_point[2] = fminf_local((*box).min_point[2], point[2]);
    (*box).max_point[0] = fmaxf_local((*box).max_point[0], point[0]);
    (*box).max_point[1] = fmaxf_local((*box).max_point[1], point[1]);
    (*box).max_point[2] = fmaxf_local((*box).max_point[2], point[2]);
}

void BvhAabb_merge(const BvhAabb *other, BvhAabb *dest) {
    if (!other || !dest) {
        return;
    }
    (*dest).min_point[0] = fminf_local((*dest).min_point[0], (*other).min_point[0]);
    (*dest).min_point[1] = fminf_local((*dest).min_point[1], (*other).min_point[1]);
    (*dest).min_point[2] = fminf_local((*dest).min_point[2], (*other).min_point[2]);
    (*dest).max_point[0] = fmaxf_local((*dest).max_point[0], (*other).max_point[0]);
    (*dest).max_point[1] = fmaxf_local((*dest).max_point[1], (*other).max_point[1]);
    (*dest).max_point[2] = fmaxf_local((*dest).max_point[2], (*other).max_point[2]);
}

bool BvhAabb_intersect_ray(const BvhRay *ray, const BvhAabb *box, float *t_near_out) {
    if (!ray || !box) {
        return false;
    }

    float t_min = (*ray).t_min;
    float t_max = (*ray).t_max;

    for (int i = 0; i < 3; i++) {
        float inv_d = ((*ray).direction[i] != 0.0f) ? (1.0f / (*ray).direction[i]) : 1e30f;
        float t0 = ((*box).min_point[i] - (*ray).origin[i]) * inv_d;
        float t1 = ((*box).max_point[i] - (*ray).origin[i]) * inv_d;
        if (inv_d < 0.0f) {
            float temp = t0;
            t0 = t1;
            t1 = temp;
        }
        t_min = (t0 > t_min) ? t0 : t_min;
        t_max = (t1 < t_max) ? t1 : t_max;
        if (t_max < t_min) {
            return false;
        }
    }

    if (t_near_out) {
        *t_near_out = t_min;
    }
    return true;
}

bool BvhTree_init(uint32_t primitive_count, BvhTree *tree) {
    if (!tree) {
        return false;
    }
    (*tree).primitive_count = primitive_count;
    (*tree).node_count = 0;
    (*tree).primitives = NULL;

    if (primitive_count == 0) {
        (*tree).nodes = NULL;
        (*tree).primitive_indices = NULL;
        (*tree).node_capacity = 0;
        return true;
    }

    uint32_t max_nodes = (2u * primitive_count) + 1u;
    (*tree).nodes = (BvhNode*) malloc(max_nodes * sizeof(BvhNode));
    (*tree).primitive_indices = (uint32_t*) malloc(primitive_count * sizeof(uint32_t));
    if (!(*tree).nodes || !(*tree).primitive_indices) {
        free((*tree).nodes);
        free((*tree).primitive_indices);
        (*tree).nodes = NULL;
        (*tree).primitive_indices = NULL;
        return false;
    }
    (*tree).node_capacity = max_nodes;
    for (uint32_t i = 0; i < primitive_count; i++) {
        (*tree).primitive_indices[i] = i;
    }
    return true;
}

void BvhTree_destroy(BvhTree *tree) {
    if (!tree) {
        return;
    }
    if ((*tree).nodes) {
        free((*tree).nodes);
        (*tree).nodes = NULL;
    }
    if ((*tree).primitive_indices) {
        free((*tree).primitive_indices);
        (*tree).primitive_indices = NULL;
    }
    (*tree).node_count = 0;
    (*tree).node_capacity = 0;
    (*tree).primitive_count = 0;
    (*tree).primitives = NULL;
}

static int32_t bvh_build_recursive(
    BvhTree *tree,
    uint32_t offset,
    uint32_t count,
    uint32_t max_leaf_prims
) {
    if (count == 0) {
        return -1;
    }

    uint32_t node_idx = (*tree).node_count++;
    BvhNode *node = &(*tree).nodes[node_idx];
    (*node).bounds = BvhAabb_empty();
    (*node).left_child = -1;
    (*node).right_child = -1;
    (*node).primitive_offset = offset;
    (*node).primitive_count = count;
    (*node).is_leaf = false;

    BvhAabb centroid_bounds = BvhAabb_empty();
    for (uint32_t i = 0; i < count; i++) {
        uint32_t prim_id = (*tree).primitive_indices[offset + i];
        const BvhPrimitive *prim = &(*tree).primitives[prim_id];
        BvhAabb_merge(&(*prim).bounds, &(*node).bounds);
        BvhAabb_expand_point((*prim).centroid, &centroid_bounds);
    }

    if (count <= max_leaf_prims) {
        (*node).is_leaf = true;
        return (int32_t) node_idx;
    }

    float extent[3];
    extent[0] = centroid_bounds.max_point[0] - centroid_bounds.min_point[0];
    extent[1] = centroid_bounds.max_point[1] - centroid_bounds.min_point[1];
    extent[2] = centroid_bounds.max_point[2] - centroid_bounds.min_point[2];

    int axis = 0;
    if (extent[1] > extent[0]) {
        axis = 1;
    }
    if (extent[2] > extent[axis]) {
        axis = 2;
    }

    if (extent[axis] <= 1e-6f) {
        (*node).is_leaf = true;
        return (int32_t) node_idx;
    }

    float split_pos = 0.5f * (centroid_bounds.min_point[axis] + centroid_bounds.max_point[axis]);

    // Partition primitives in-place
    int32_t left = (int32_t) offset;
    int32_t right = (int32_t) (offset + count - 1);

    while (left <= right) {
        uint32_t prim_id = (*tree).primitive_indices[left];
        if ((*tree).primitives[prim_id].centroid[axis] < split_pos) {
            left++;
        } else {
            uint32_t temp = (*tree).primitive_indices[left];
            (*tree).primitive_indices[left] = (*tree).primitive_indices[right];
            (*tree).primitive_indices[right] = temp;
            right--;
        }
    }

    uint32_t left_count = (uint32_t) (left - (int32_t) offset);
    if (left_count == 0 || left_count == count) {
        left_count = count / 2;
    }

    int32_t left_child = bvh_build_recursive(tree, offset, left_count, max_leaf_prims);
    int32_t right_child = bvh_build_recursive(tree, offset + left_count, count - left_count, max_leaf_prims);

    // Re-acquire node pointer in case memory relocated (though capacity was pre-allocated)
    node = &(*tree).nodes[node_idx];
    (*node).left_child = left_child;
    (*node).right_child = right_child;
    (*node).is_leaf = false;

    return (int32_t) node_idx;
}

bool BvhTree_build(const BvhPrimitive *prims, uint32_t count, BvhTree *tree) {
    if (!tree || !prims) {
        return false;
    }
    if (!BvhTree_init(count, tree)) {
        return false;
    }
    if (count == 0) {
        return true;
    }

    (*tree).primitives = prims;
    bvh_build_recursive(tree, 0, count, 2);
    return true;
}

bool BvhTree_intersect_ray(const BvhTree *tree, const BvhRay *ray, BvhRayHit *hit_out) {
    if (!tree || !ray || !hit_out || (*tree).node_count == 0) {
        if (hit_out) {
            (*hit_out).hit = false;
        }
        return false;
    }

    (*hit_out).hit = false;
    (*hit_out).t = (*ray).t_max;
    (*hit_out).primitive_id = 0;
    (*hit_out).normal[0] = 0.0f;
    (*hit_out).normal[1] = 0.0f;
    (*hit_out).normal[2] = 1.0f;

    int32_t stack[64];
    int stack_ptr = 0;
    stack[stack_ptr++] = 0;

    while (stack_ptr > 0) {
        int32_t node_idx = stack[--stack_ptr];
        const BvhNode *node = &(*tree).nodes[node_idx];

        float t_box = 0.0f;
        BvhRay curr_ray = *ray;
        curr_ray.t_max = (*hit_out).t;

        if (!BvhAabb_intersect_ray(&curr_ray, &(*node).bounds, &t_box)) {
            continue;
        }

        if ((*node).is_leaf) {
            for (uint32_t i = 0; i < (*node).primitive_count; i++) {
                uint32_t prim_idx = (*tree).primitive_indices[(*node).primitive_offset + i];
                const BvhPrimitive *prim = &(*tree).primitives[prim_idx];

                float t_prim = 0.0f;
                if (BvhAabb_intersect_ray(&curr_ray, &(*prim).bounds, &t_prim)) {
                    if (t_prim < (*hit_out).t) {
                        (*hit_out).hit = true;
                        (*hit_out).t = t_prim;
                        (*hit_out).primitive_id = (*prim).id;
                    }
                }
            }
        } else {
            if ((*node).left_child >= 0 && stack_ptr < 64) {
                stack[stack_ptr++] = (*node).left_child;
            }
            if ((*node).right_child >= 0 && stack_ptr < 64) {
                stack[stack_ptr++] = (*node).right_child;
            }
        }
    }

    return (*hit_out).hit;
}
