#ifndef ALGO_KD_TREE_H
#define ALGO_KD_TREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// algo/kd_tree.h — 3-Dimensional K-D Tree for Spatial Search.
//
// Organizes 3D points by cycling coordinate split planes (X -> Y -> Z)
// to accelerate nearest-neighbor queries and spherical proximity searches.

typedef struct KdPoint {
    float    coord[3]; // [0] = x/horizontal, [1] = y/vertical, [2] = z/depth
    uint64_t payload;
} KdPoint;

typedef struct KdNode {
    KdPoint        point;
    uint8_t        axis; // 0 = X, 1 = Y, 2 = Z
    struct KdNode *left;
    struct KdNode *right;
} KdNode;

typedef struct KdTree {
    KdNode *root;
    size_t  count;
} KdTree;

// Build a balanced KD-tree from an array of 3D points
KdTree *KdTree_build(KdPoint *points, size_t count);
void    KdTree_free(KdTree *tree);

// Find single nearest neighbor to query point.
// Dest-last order: writes to outNearest and outDistanceSquared.
bool   KdTree_nearest(const KdTree *tree, const float queryCoord[3], KdPoint *outNearest, float *outDistSq);

// Find all points within radius of query point.
size_t KdTree_queryRadius(const KdTree *tree, const float queryCoord[3], float radius, uint64_t *outPayloads, size_t maxCount);

#endif
