#include "algo/kd_tree.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: KdTree
 * ============================================================================
 * Spatial acceleration structure for 3D point clouds: recursively partitions
 * points by alternating X/Y/Z split planes so nearest-neighbor and radius
 * queries run in O(log N) expected time instead of a linear scan. The tree is
 * built once from a caller-owned point array (arena-allocated nodes, balanced
 * via quickselect on the median), then queried read-only; KdTree_free walks
 * the arena nodes and releases the tree struct. Lives at R2 as a pure leaf
 * math/geometry primitive with zero dependencies above it.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: KdTree (algo/kd_tree.c)
 * LEVEL: L2 — 3D Spatial Partitioning KD-Tree
 * ============================================================================
 * Recursively partitions 3D space by alternating split axes (X, Y, Z)
 * enabling O(log N) nearest-neighbor and spherical radius queries.
 *
 * STRUCT FIELDS (Mirroring algo/kd_tree.h):
 * ----------------------------------------------------------------------------
 *   KdPoint {
 *     float coord[3];    // [0] = x/horizontal, [1] = y/vertical, [2] = z/depth
 *     uint64_t payload;  // caller payload carried by the point
 *   }
 *   KdNode {
 *     KdPoint point;         // split point stored at this node
 *     uint8_t axis;          // 0 = X, 1 = Y, 2 = Z split axis
 *     struct KdNode *left;   // subtree on the near side of the split plane
 *     struct KdNode *right;  // subtree on the far side of the split plane
 *   }
 *   KdTree {
 *     KdNode *root;  // root of the balanced tree (nullptr when empty)
 *     size_t count;  // number of points indexed
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - KdTree_build(points, count)
 *   - KdTree_free(tree)
 *   - KdTree_nearest(tree, queryCoord, outNearest, outDistSq)
 *   - KdTree_queryRadius(tree, queryCoord, radius, outPayloads, maxCount)
 * Private Core Functions: (.c static)
 *   - swapPoints(a, b)
 *   - partitionAxis(points, left, right, axis)
 *   - quickSelectAxis(points, left, right, k, axis)
 *   - KdTree_buildRecursive(points, left, right, depth)
 *   - KdNode_freeRecursive(node)
 *   - distSq(a, b)
 *   - nearestRecursive(node, query, bestPoint, bestDistSq)
 *   - radiusRecursive(node, query, radiusSq, outPayloads, maxCount, count)
 * ============================================================================
 */

static void swapPoints(KdPoint *a, KdPoint *b) {
    KdPoint tmp = *a;
    *a = *b;
    *b = tmp;
}

static size_t partitionAxis(KdPoint *points, size_t left, size_t right, uint8_t axis) {
    float pivot = points[right].coord[axis];
    size_t i = left;
    for (size_t j = left; j < right; j++) {
        if (points[j].coord[axis] < pivot) {
            swapPoints(&points[i], &points[j]);
            i++;
        }
    }
    swapPoints(&points[i], &points[right]);
    return i;
}

static void quickSelectAxis(KdPoint *points, size_t left, size_t right, size_t k, uint8_t axis) {
    while (left < right) {
        size_t pivotIdx = partitionAxis(points, left, right, axis);
        if (pivotIdx == k) {
            return;
        } else if (k < pivotIdx) {
            if (pivotIdx == 0) break;
            right = pivotIdx - 1;
        } else {
            left = pivotIdx + 1;
        }
    }
}

static KdNode *KdTree_buildRecursive(KdPoint *points, size_t left, size_t right, uint8_t depth) {
    if (left > right) return nullptr;

    uint8_t axis = depth % 3;
    size_t mid = left + (right - left) / 2;

    quickSelectAxis(points, left, right, mid, axis);

    KdNode *node = (KdNode*) Memory_alloc(TYPE_BYTE_ARRAY, sizeof(KdNode));
    if (node == nullptr) return nullptr;

    (*node).point = points[mid];
    (*node).axis = axis;
    (*node).left = (mid > left) ? KdTree_buildRecursive(points, left, mid - 1, depth + 1) : nullptr;
    (*node).right = (mid < right) ? KdTree_buildRecursive(points, mid + 1, right, depth + 1) : nullptr;

    return node;
}

KdTree *KdTree_build(KdPoint *points, size_t count) {
    if (points == nullptr || count == 0) return nullptr;

    KdTree *tree = (KdTree*) Memory_alloc(TYPE_BYTE_ARRAY, sizeof(KdTree));
    if (tree == nullptr) return nullptr;

    (*tree).count = count;
    (*tree).root = KdTree_buildRecursive(points, 0, count - 1, 0);
    return tree;
}

static void KdNode_freeRecursive(KdNode *node) {
    if (node == nullptr) return;
    if ((*node).left != nullptr) KdNode_freeRecursive((*node).left);
    if ((*node).right != nullptr) KdNode_freeRecursive((*node).right);
    Memory_free(node);
}

void KdTree_free(KdTree *tree) {
    if (tree == nullptr) return;
    if ((*tree).root != nullptr) {
        KdNode_freeRecursive((*tree).root);
        (*tree).root = nullptr;
    }
    Memory_free(tree);
}

static inline float distSq(const float a[3], const float b[3]) {
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return dx * dx + dy * dy + dz * dz;
}

static void nearestRecursive(const KdNode *node,
                             const float   query[3],
                             KdPoint      *bestPoint,
                             float        *bestDistSq) {
    if (node == nullptr) return;

    float d = distSq(query, (*node).point.coord);
    if (d < *bestDistSq) {
        *bestDistSq = d;
        *bestPoint = (*node).point;
    }

    uint8_t axis = (*node).axis;
    float planeDelta = query[axis] - (*node).point.coord[axis];

    const KdNode *nearChild = (planeDelta <= 0.0f) ? (*node).left : (*node).right;
    const KdNode *farChild  = (planeDelta <= 0.0f) ? (*node).right : (*node).left;

    // Search near side first
    nearestRecursive(nearChild, query, bestPoint, bestDistSq);

    // Check if far side could possibly contain a closer point
    if (planeDelta * planeDelta < *bestDistSq) {
        nearestRecursive(farChild, query, bestPoint, bestDistSq);
    }
}

bool KdTree_nearest(const KdTree *tree, const float queryCoord[3], KdPoint *outNearest, float *outDistSq) {
    if (tree == nullptr || (*tree).root == nullptr || outNearest == nullptr) {
        return false;
    }
    float bestD = 1e30f;
    KdPoint bestP = (*(*tree).root).point;
    nearestRecursive((*tree).root, queryCoord, &bestP, &bestD);

    *outNearest = bestP;
    if (outDistSq != nullptr) {
        *outDistSq = bestD;
    }
    return true;
}

static size_t radiusRecursive(const KdNode *node,
                              const float   query[3],
                              float         radiusSq,
                              uint64_t     *outPayloads,
                              size_t        maxCount,
                              size_t        count) {
    if (node == nullptr || count >= maxCount) return count;

    float d = distSq(query, (*node).point.coord);
    if (d <= radiusSq) {
        outPayloads[count++] = (*node).point.payload;
        if (count >= maxCount) return count;
    }

    uint8_t axis = (*node).axis;
    float planeDelta = query[axis] - (*node).point.coord[axis];

    const KdNode *nearChild = (planeDelta <= 0.0f) ? (*node).left : (*node).right;
    const KdNode *farChild  = (planeDelta <= 0.0f) ? (*node).right : (*node).left;

    count = radiusRecursive(nearChild, query, radiusSq, outPayloads, maxCount, count);

    if (count < maxCount && (planeDelta * planeDelta <= radiusSq)) {
        count = radiusRecursive(farChild, query, radiusSq, outPayloads, maxCount, count);
    }

    return count;
}

size_t KdTree_queryRadius(const KdTree *tree, const float queryCoord[3], float radius, uint64_t *outPayloads, size_t maxCount) {
    if (tree == nullptr || (*tree).root == nullptr || outPayloads == nullptr || maxCount == 0 || radius < 0.0f) {
        return 0;
    }
    return radiusRecursive((*tree).root, queryCoord, radius * radius, outPayloads, maxCount, 0);
}
