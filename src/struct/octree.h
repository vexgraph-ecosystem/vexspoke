#ifndef STRUCT_OCTREE_H
#define STRUCT_OCTREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

typedef struct OctreeAABB {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
} OctreeAABB;

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
} Octree;

Octree *Octree_create(OctreeAABB bounds, uint32_t maxDepth, uint32_t maxItemsPerNode);
void Octree_free(Octree *self);

bool Octree_insert(Octree *self, OctreePoint point, uint64_t payload);
size_t Octree_queryRange(const Octree *self, OctreeAABB range, uint64_t *outPayloads, size_t maxCount);
size_t Octree_querySphere(const Octree *self, OctreePoint center, float radius, uint64_t *outPayloads, size_t maxCount);

size_t Octree_count(const Octree *self);
void Octree_clear(Octree *self);

// AABB helper functions
bool OctreeAABB_containsPoint(OctreeAABB box, OctreePoint p);
bool OctreeAABB_intersects(OctreeAABB a, OctreeAABB b);
bool OctreeAABB_intersectsSphere(OctreeAABB box, OctreePoint center, float radius);

#endif
