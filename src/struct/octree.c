#include "struct/octree.h"

#include <string.h>
#include <math.h>
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Octree
 * ============================================================================
 * 3D spatial partitioning octree. Partitions 3D volumes into 8 octants to
 * accelerate range queries, frustum/AABB intersections, and radius queries.
 * Arena-allocated: the Octree owns a root OctreeNode; leaves grow their item
 * arrays by doubling and subdivide into eight children once full. Insert and
 * query recurse through the octant tree with bounded depth, keeping point
 * lookups O(log N) on average.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Octree (struct/octree.c)
 * LEVEL: L2 — Off-Heap Spatial Tree
 * ============================================================================
 * 3D Spatial Partitioning Octree.
 *
 * Partitions 3D volumes into 8 octants to accelerate range queries,
 * frustum/AABB intersections, and radius queries.
 *
 * STRUCT FIELDS (Mirroring struct/octree.h):
 * ----------------------------------------------------------------------------
 *   OctreePoint {
 *     float x;
 *     float y;
 *     float z;
 *   }
 *   OctreeAABB {
 *     float minX; float minY; float minZ;
 *     float maxX; float maxY; float maxZ;
 *   }
 *   OctreeItem {
 *     OctreePoint point;
 *     uint64_t    payload;
 *   }
 *   OctreeNode {
 *     OctreeAABB         bounds;
 *     OctreeItem        *items;
 *     uint32_t           itemCount;
 *     uint32_t           itemCapacity;
 *     bool               isLeaf;
 *     struct OctreeNode *children[8];
 *   }
 *   Octree {
 *     OctreeNode *root;
 *     uint32_t    maxDepth;
 *     uint32_t    maxItemsPerNode;
 *     size_t      totalItems;
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - Octree_create(bounds, maxDepth, maxItemsPerNode)
 *
 * Public Core Functions: (.h)
 *   - Octree_free(self)
 *   - Octree_insert(self, point, payload)
 *   - Octree_queryRange(self, range, outPayloads, maxCount)
 *   - Octree_querySphere(self, center, radius, outPayloads, maxCount)
 *   - Octree_count(self)
 *   - Octree_clear(self)
 *   - OctreeAABB_containsPoint(box, p)
 *   - OctreeAABB_intersects(a, b)
 *   - OctreeAABB_intersectsSphere(box, center, radius)
 *
 * Private Core Functions: (.c static)
 *   - OctreeNode_create(bounds, capacity)
 *   - OctreeNode_free(node)
 *   - OctreeNode_subdivide(node, childCapacity)
 *   - OctreeNode_insert(node, item, depth, maxDepth, maxItems)
 *   - OctreeNode_queryRange(node, range, outPayloads, maxCount, count)
 *   - OctreeNode_querySphere(node, center, radius, outPayloads, maxCount, count)
 * ============================================================================
 */

bool OctreeAABB_containsPoint(OctreeAABB box, OctreePoint p) {
    return (p.x >= box.minX && p.x <= box.maxX &&
            p.y >= box.minY && p.y <= box.maxY &&
            p.z >= box.minZ && p.z <= box.maxZ);
}

bool OctreeAABB_intersects(OctreeAABB a, OctreeAABB b) {
    return (a.minX <= b.maxX && a.maxX >= b.minX &&
            a.minY <= b.maxY && a.maxY >= b.minY &&
            a.minZ <= b.maxZ && a.maxZ >= b.minZ);
}

bool OctreeAABB_intersectsSphere(OctreeAABB box, OctreePoint center, float radius) {
    float closestX = center.x;
    if (closestX < box.minX) closestX = box.minX;
    else if (closestX > box.maxX) closestX = box.maxX;

    float closestY = center.y;
    if (closestY < box.minY) closestY = box.minY;
    else if (closestY > box.maxY) closestY = box.maxY;

    float closestZ = center.z;
    if (closestZ < box.minZ) closestZ = box.minZ;
    else if (closestZ > box.maxZ) closestZ = box.maxZ;

    float dx = center.x - closestX;
    float dy = center.y - closestY;
    float dz = center.z - closestZ;
    return (dx * dx + dy * dy + dz * dz) <= (radius * radius);
}

static OctreeNode *OctreeNode_create(OctreeAABB bounds, uint32_t capacity) {
    OctreeNode *node = (OctreeNode*) Memory_alloc(TYPE_OCTREE, sizeof(OctreeNode));
    if (node == nullptr) {
        return nullptr;
    }
    (*node).bounds = bounds;
    (*node).itemCount = 0;
    (*node).itemCapacity = capacity;
    (*node).isLeaf = true;
    for (int i = 0; i < 8; i++) {
        (*node).children[i] = nullptr;
    }
    (*node).items = (OctreeItem*) Memory_alloc(TYPE_BYTE_ARRAY, (size_t) capacity * sizeof(OctreeItem));
    if ((*node).items == nullptr) {
        Memory_free(node);
        return nullptr;
    }
    return node;
}

static void OctreeNode_free(OctreeNode *node) {
    if (node == nullptr) {
        return;
    }
    if (!(*node).isLeaf) {
        for (int i = 0; i < 8; i++) {
            if ((*node).children[i] != nullptr) {
                OctreeNode_free((*node).children[i]);
                (*node).children[i] = nullptr;
            }
        }
    }
    if ((*node).items != nullptr) {
        Memory_free((*node).items);
        (*node).items = nullptr;
    }
    Memory_free(node);
}

static void OctreeNode_subdivide(OctreeNode *node, uint32_t childCapacity) {
    float midX = ((*node).bounds.minX + (*node).bounds.maxX) * 0.5f;
    float midY = ((*node).bounds.minY + (*node).bounds.maxY) * 0.5f;
    float midZ = ((*node).bounds.minZ + (*node).bounds.maxZ) * 0.5f;

    OctreeAABB subBounds[8];
    // 0: ---
    subBounds[0] = (OctreeAABB) { (*node).bounds.minX, (*node).bounds.minY, (*node).bounds.minZ, midX, midY, midZ };
    // 1: +--
    subBounds[1] = (OctreeAABB) { midX, (*node).bounds.minY, (*node).bounds.minZ, (*node).bounds.maxX, midY, midZ };
    // 2: -+-
    subBounds[2] = (OctreeAABB) { (*node).bounds.minX, midY, (*node).bounds.minZ, midX, (*node).bounds.maxY, midZ };
    // 3: ++-
    subBounds[3] = (OctreeAABB) { midX, midY, (*node).bounds.minZ, (*node).bounds.maxX, (*node).bounds.maxY, midZ };
    // 4: --+
    subBounds[4] = (OctreeAABB) { (*node).bounds.minX, (*node).bounds.minY, midZ, midX, midY, (*node).bounds.maxZ };
    // 5: +-+
    subBounds[5] = (OctreeAABB) { midX, (*node).bounds.minY, midZ, (*node).bounds.maxX, midY, (*node).bounds.maxZ };
    // 6: -++
    subBounds[6] = (OctreeAABB) { (*node).bounds.minX, midY, midZ, midX, (*node).bounds.maxY, (*node).bounds.maxZ };
    // 7: +++
    subBounds[7] = (OctreeAABB) { midX, midY, midZ, (*node).bounds.maxX, (*node).bounds.maxY, (*node).bounds.maxZ };

    for (int i = 0; i < 8; i++) {
        (*node).children[i] = OctreeNode_create(subBounds[i], childCapacity);
    }
    (*node).isLeaf = false;
}

static bool OctreeNode_insert(OctreeNode *node, OctreeItem item, uint32_t depth, uint32_t maxDepth, uint32_t maxItems) {
    if (!OctreeAABB_containsPoint((*node).bounds, item.point)) {
        return false;
    }

    if ((*node).isLeaf) {
        if ((*node).itemCount < maxItems || depth >= maxDepth) {
            // Can add directly to this leaf
            if ((*node).itemCount >= (*node).itemCapacity) {
                // Grow leaf capacity
                uint32_t newCap = (*node).itemCapacity * 2;
                if (newCap == 0) newCap = 8;
                OctreeItem *newItems = (OctreeItem*) Memory_alloc(TYPE_BYTE_ARRAY, (size_t) newCap * sizeof(OctreeItem));
                if (newItems == nullptr) return false;
                if ((*node).items != nullptr) {
                    memcpy(newItems, (*node).items, (size_t) (*node).itemCount * sizeof(OctreeItem));
                    Memory_free((*node).items);
                }
                (*node).items = newItems;
                (*node).itemCapacity = newCap;
            }
            (*node).items[(*node).itemCount] = item;
            (*node).itemCount++;
            return true;
        }

        // Subdivide leaf into 8 octants
        OctreeNode_subdivide(node, maxItems);

        // Re-insert existing items into children
        for (uint32_t i = 0; i < (*node).itemCount; i++) {
            OctreeItem existing = (*node).items[i];
            for (int c = 0; c < 8; c++) {
                if (OctreeNode_insert((*node).children[c], existing, depth + 1, maxDepth, maxItems)) {
                    break;
                }
            }
        }
        (*node).itemCount = 0;
    }

    // Insert new item into matching child
    for (int c = 0; c < 8; c++) {
        if (OctreeNode_insert((*node).children[c], item, depth + 1, maxDepth, maxItems)) {
            return true;
        }
    }

    return false;
}

Octree *Octree_create(OctreeAABB bounds, uint32_t maxDepth, uint32_t maxItemsPerNode) {
    if (maxDepth == 0) maxDepth = OCTREE_DEFAULT_MAX_DEPTH;
    if (maxItemsPerNode == 0) maxItemsPerNode = OCTREE_DEFAULT_NODE_CAPACITY;

    Octree *self = (Octree*) Memory_alloc(TYPE_OCTREE, sizeof(Octree));
    if (self == nullptr) {
        return nullptr;
    }

    (*self).maxDepth = maxDepth;
    (*self).maxItemsPerNode = maxItemsPerNode;
    (*self).totalItems = 0;
    (*self).root = OctreeNode_create(bounds, maxItemsPerNode);
    if ((*self).root == nullptr) {
        Memory_free(self);
        return nullptr;
    }

    return self;
}

void Octree_free(Octree *self) {
    if (self == nullptr) {
        return;
    }
    if ((*self).root != nullptr) {
        OctreeNode_free((*self).root);
        (*self).root = nullptr;
    }
    Memory_free(self);
}

bool Octree_insert(Octree *self, OctreePoint point, uint64_t payload) {
    if (self == nullptr || (*self).root == nullptr) {
        return false;
    }
    OctreeItem item;
    item.point = point;
    item.payload = payload;

    bool ok = OctreeNode_insert((*self).root, item, 0, (*self).maxDepth, (*self).maxItemsPerNode);
    if (ok) {
        (*self).totalItems++;
    }
    return ok;
}

static size_t OctreeNode_queryRange(const OctreeNode *node, OctreeAABB range, uint64_t *outPayloads, size_t maxCount, size_t count) {
    if (node == nullptr || count >= maxCount) {
        return count;
    }
    if (!OctreeAABB_intersects((*node).bounds, range)) {
        return count;
    }

    if ((*node).isLeaf) {
        for (uint32_t i = 0; i < (*node).itemCount && count < maxCount; i++) {
            if (OctreeAABB_containsPoint(range, (*node).items[i].point)) {
                outPayloads[count++] = (*node).items[i].payload;
            }
        }
        return count;
    }

    for (int i = 0; i < 8 && count < maxCount; i++) {
        count = OctreeNode_queryRange((*node).children[i], range, outPayloads, maxCount, count);
    }
    return count;
}

size_t Octree_queryRange(const Octree *self, OctreeAABB range, uint64_t *outPayloads, size_t maxCount) {
    if (self == nullptr || (*self).root == nullptr || outPayloads == nullptr || maxCount == 0) {
        return 0;
    }
    return OctreeNode_queryRange((*self).root, range, outPayloads, maxCount, 0);
}

static size_t OctreeNode_querySphere(const OctreeNode *node, OctreePoint center, float radius, uint64_t *outPayloads, size_t maxCount, size_t count) {
    if (node == nullptr || count >= maxCount) {
        return count;
    }
    if (!OctreeAABB_intersectsSphere((*node).bounds, center, radius)) {
        return count;
    }

    float rSq = radius * radius;
    if ((*node).isLeaf) {
        for (uint32_t i = 0; i < (*node).itemCount && count < maxCount; i++) {
            float dx = (*node).items[i].point.x - center.x;
            float dy = (*node).items[i].point.y - center.y;
            float dz = (*node).items[i].point.z - center.z;
            if (dx * dx + dy * dy + dz * dz <= rSq) {
                outPayloads[count++] = (*node).items[i].payload;
            }
        }
        return count;
    }

    for (int i = 0; i < 8 && count < maxCount; i++) {
        count = OctreeNode_querySphere((*node).children[i], center, radius, outPayloads, maxCount, count);
    }
    return count;
}

size_t Octree_querySphere(const Octree *self, OctreePoint center, float radius, uint64_t *outPayloads, size_t maxCount) {
    if (self == nullptr || (*self).root == nullptr || outPayloads == nullptr || maxCount == 0) {
        return 0;
    }
    return OctreeNode_querySphere((*self).root, center, radius, outPayloads, maxCount, 0);
}

size_t Octree_count(const Octree *self) {
    if (self == nullptr) return 0;
    return (*self).totalItems;
}

void Octree_clear(Octree *self) {
    if (self == nullptr || (*self).root == nullptr) return;
    OctreeAABB bounds = (*(*self).root).bounds;
    OctreeNode_free((*self).root);
    (*self).root = OctreeNode_create(bounds, (*self).maxItemsPerNode);
    (*self).totalItems = 0;
}
