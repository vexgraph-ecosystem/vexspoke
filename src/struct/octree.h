#ifndef STRUCT_OCTREE_H
#define STRUCT_OCTREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "c23/constructor.h"

#include "struct/exhaustion.h"

// struct/octree.h — 3D Spatial Partitioning Octree.
//
// Recursively partitions 3D space into 8 sub-octants for O(log N) point insertion,
// bounding-box (AABB) intersection range queries, and spherical range queries.

#define OCTREE_DEFAULT_MAX_DEPTH 8
#define OCTREE_DEFAULT_NODE_CAPACITY 8

typedef struct OctreePoint {
    float x;
    float y;
    float z;
} OctreePoint;

static inline OctreePoint OctreePoint_3(float x, float y, float z) {
    return (OctreePoint) { .x = x, .y = y, .z = z };
}
#define OctreePoint(...) CONSTRUCTOR_DISPATCH(OctreePoint, __VA_ARGS__)
static inline void OctreePoint_set(OctreePoint *point, float x, float y, float z) {
    if (point) { (*point).x = x; (*point).y = y; (*point).z = z; }
}

typedef struct OctreeAABB {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
} OctreeAABB;

static inline OctreeAABB OctreeAABB_6(float minX, float minY, float minZ,
                                      float maxX, float maxY, float maxZ) {
    return (OctreeAABB) { .minX = minX, .minY = minY, .minZ = minZ,
        .maxX = maxX, .maxY = maxY, .maxZ = maxZ };
}
#define OctreeAABB(...) CONSTRUCTOR_DISPATCH(OctreeAABB, __VA_ARGS__)
static inline void OctreeAABB_setMin(OctreeAABB *box, float x, float y, float z) {
    if (box) { (*box).minX = x; (*box).minY = y; (*box).minZ = z; }
}
static inline void OctreeAABB_setMax(OctreeAABB *box, float x, float y, float z) {
    if (box) { (*box).maxX = x; (*box).maxY = y; (*box).maxZ = z; }
}

typedef struct OctreeItem {
    OctreePoint point;
    uint64_t    payload;
} OctreeItem;

typedef struct OctreeNode {
    OctreeAABB         bounds;
    OctreeItem        *items;
    uint32_t           itemCount;
    uint32_t           itemCapacity;
    bool               isLeaf;
    struct OctreeNode *children[8];
} OctreeNode;

typedef struct Octree {
    OctreeNode *root;
    uint32_t    maxDepth;
    uint32_t    maxItemsPerNode;
    size_t      totalItems;
    // Exhaustion observability (the Exhaustion Loudness Law): a refused leaf
    // growth is counted, and the epoch's first refusal reports.
    uint64_t    exhaustionCount;
    bool        exhaustionReported;
} Octree;

Octree *Octree_3(OctreeAABB bounds, uint32_t maxDepth, uint32_t maxItemsPerNode);
#define Octree(...) CONSTRUCTOR_DISPATCH(Octree, __VA_ARGS__)
void Octree_free(Octree *self);

bool Octree_insert(Octree *self, OctreePoint point, uint64_t payload);
size_t Octree_queryRange(const Octree *self, OctreeAABB range, uint64_t *outPayloads, size_t maxCount);
size_t Octree_querySphere(const Octree *self, OctreePoint center, float radius, uint64_t *outPayloads, size_t maxCount);

size_t Octree_count(const Octree *self);
void Octree_clear(Octree *self);
// Refused-insertion count since creation (the Exhaustion Loudness Law).
;;TEST
uint64_t Octree_exhaustionCount(const Octree *self);

// AABB helper functions
bool OctreeAABB_containsPoint(OctreeAABB box, OctreePoint p);
bool OctreeAABB_intersects(OctreeAABB a, OctreeAABB b);
bool OctreeAABB_intersectsSphere(OctreeAABB box, OctreePoint center, float radius);

#endif
