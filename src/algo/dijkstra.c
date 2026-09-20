#include "algo/dijkstra.h"

#include <stdlib.h>
#include <string.h>
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Dijkstra
 * ============================================================================
 * Single-source shortest-path solver over weighted adjacency lists, used for
 * routing, spatial navigation, and audio node dependency graphs. The graph is
 * a pair of contiguous off-heap arenas (nodes plus per-node edge arrays) so
 * relaxation walks cache-friendly memory; edge arrays double on demand and
 * are reclaimed with the graph. The priority queue is a private min-heap over
 * a caller-budgeted scratch arena sized 4N+16, so the hot search path performs
 * zero steady-state allocation. Unreachable goals return 0 with all scratch
 * freed; the path is reconstructed backward through the predecessor table and
 * reversed into the caller's dest-last outPath buffer.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Dijkstra (algo/dijkstra.c)
 * LEVEL: L2 — Graph Shortest-Path Engine
 * ============================================================================
 * Solves single-source shortest paths on adjacency lists using min-priority
 * relaxation. Operates over contiguous off-heap memory.
 *
 * STRUCT FIELDS (Mirroring algo/dijkstra.h):
 * ----------------------------------------------------------------------------
 *   DijkstraEdge {
 *     uint32_t target;   // destination node index
 *     float weight;      // edge cost
 *   }
 *   DijkstraNode {
 *     uint32_t edgeCount;     // live outgoing edges
 *     uint32_t edgeCapacity;  // allocated edge slots (doubling)
 *     DijkstraEdge *edges;    // contiguous edge arena
 *   }
 *   DijkstraGraph {
 *     uint32_t nodeCount;  // fixed node count at creation
 *     DijkstraNode *nodes; // contiguous node arena
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - Dijkstra_create(nodeCount)
 *
 * Public Core Functions: (.h)
 *   - Dijkstra_free(graph)
 *   - Dijkstra_addEdge(graph, from, to, weight)
 *   - Dijkstra_addBiEdge(graph, a, b, weight)
 *   - Dijkstra_shortestPath(graph, startNode, goalNode, outPath, maxPathNodes, outTotalDistance)
 *
 * Private Core Functions: (.c static)
 *   - MinQ_push(q, node, dist)
 *   - MinQ_pop(q, outNode, outDist)
 * ============================================================================
 */

DijkstraGraph *Dijkstra_create(uint32_t nodeCount) {
    if (nodeCount == 0) return nullptr;

    DijkstraGraph *graph = (DijkstraGraph*) Memory_alloc(TYPE_BYTE_ARRAY, sizeof(DijkstraGraph));
    if (graph == nullptr) return nullptr;

    (*graph).nodeCount = nodeCount;
    size_t nodesBytes = (size_t) nodeCount * sizeof(DijkstraNode);
    (*graph).nodes = (DijkstraNode*) Memory_alloc(TYPE_BYTE_ARRAY, nodesBytes);
    if ((*graph).nodes == nullptr) {
        Memory_free(graph);
        return nullptr;
    }

    for (uint32_t i = 0; i < nodeCount; i++) {
        (*graph).nodes[i].edgeCount = 0;
        (*graph).nodes[i].edgeCapacity = 0;
        (*graph).nodes[i].edges = nullptr;
    }

    return graph;
}

void Dijkstra_free(DijkstraGraph *graph) {
    if (graph == nullptr) return;
    if ((*graph).nodes != nullptr) {
        for (uint32_t i = 0; i < (*graph).nodeCount; i++) {
            if ((*graph).nodes[i].edges != nullptr) {
                Memory_free((*graph).nodes[i].edges);
                (*graph).nodes[i].edges = nullptr;
            }
        }
        Memory_free((*graph).nodes);
        (*graph).nodes = nullptr;
    }
    Memory_free(graph);
}

bool Dijkstra_addEdge(DijkstraGraph *graph, uint32_t from, uint32_t to, float weight) {
    if (graph == nullptr || from >= (*graph).nodeCount || to >= (*graph).nodeCount) {
        return false;
    }
    DijkstraNode *node = &(*graph).nodes[from];
    if ((*node).edgeCount >= (*node).edgeCapacity) {
        uint32_t newCap = ((*node).edgeCapacity == 0) ? 4 : (*node).edgeCapacity * 2;
        DijkstraEdge *newEdges = (DijkstraEdge*) Memory_alloc(TYPE_BYTE_ARRAY, (size_t) newCap * sizeof(DijkstraEdge));
        if (newEdges == nullptr) return false;
        if ((*node).edges != nullptr) {
            memcpy(newEdges, (*node).edges, (size_t) (*node).edgeCount * sizeof(DijkstraEdge));
            Memory_free((*node).edges);
        }
        (*node).edges = newEdges;
        (*node).edgeCapacity = newCap;
    }

    (*node).edges[(*node).edgeCount].target = to;
    (*node).edges[(*node).edgeCount].weight = weight;
    (*node).edgeCount++;
    return true;
}

bool Dijkstra_addBiEdge(DijkstraGraph *graph, uint32_t a, uint32_t b, float weight) {
    bool ok1 = Dijkstra_addEdge(graph, a, b, weight);
    bool ok2 = Dijkstra_addEdge(graph, b, a, weight);
    return ok1 && ok2;
}

// Min-heap entry for Dijkstra priority queue
typedef struct QItem {
    uint32_t node;
    float    dist;
} QItem;

typedef struct MinQ {
    QItem   *items;
    uint32_t size;
    uint32_t capacity;
} MinQ;

static void MinQ_push(MinQ *q, uint32_t node, float dist) {
    if ((*q).size >= (*q).capacity) return;
    uint32_t i = (*q).size++;
    (*q).items[i].node = node;
    (*q).items[i].dist = dist;

    // Sift up
    while (i > 0) {
        uint32_t parent = (i - 1) / 2;
        if ((*q).items[i].dist < (*q).items[parent].dist) {
            QItem swap = (*q).items[i];
            (*q).items[i] = (*q).items[parent];
            (*q).items[parent] = swap;
            i = parent;
        } else {
            break;
        }
    }
}

static bool MinQ_pop(MinQ *q, uint32_t *outNode, float *outDist) {
    if ((*q).size == 0) return false;
    *outNode = (*q).items[0].node;
    *outDist = (*q).items[0].dist;

    (*q).size--;
    if ((*q).size > 0) {
        (*q).items[0] = (*q).items[(*q).size];
        // Sift down
        uint32_t i = 0;
        while (true) {
            uint32_t left = 2 * i + 1;
            uint32_t right = 2 * i + 2;
            uint32_t smallest = i;

            if (left < (*q).size && (*q).items[left].dist < (*q).items[smallest].dist) {
                smallest = left;
            }
            if (right < (*q).size && (*q).items[right].dist < (*q).items[smallest].dist) {
                smallest = right;
            }
            if (smallest != i) {
                QItem swap = (*q).items[i];
                (*q).items[i] = (*q).items[smallest];
                (*q).items[smallest] = swap;
                i = smallest;
            } else {
                break;
            }
        }
    }
    return true;
}

size_t Dijkstra_shortestPath(const DijkstraGraph *graph,
                             uint32_t             startNode,
                             uint32_t             goalNode,
                             uint32_t            *outPath,
                             size_t               maxPathNodes,
                             float               *outTotalDistance) {
    if (graph == nullptr || outPath == nullptr || maxPathNodes == 0) return 0;
    uint32_t N = (*graph).nodeCount;
    if (startNode >= N || goalNode >= N) return 0;

    float *dist = (float*) Memory_alloc(TYPE_BYTE_ARRAY, (size_t) N * sizeof(float));
    uint32_t *prev = (uint32_t*) Memory_alloc(TYPE_BYTE_ARRAY, (size_t) N * sizeof(uint32_t));
    bool *visited = (bool*) Memory_alloc(TYPE_BYTE_ARRAY, (size_t) N * sizeof(bool));
    QItem *qItems = (QItem*) Memory_alloc(TYPE_BYTE_ARRAY, (size_t) (N * 4 + 16) * sizeof(QItem));

    if (!dist || !prev || !visited || !qItems) {
        if (dist) Memory_free(dist);
        if (prev) Memory_free(prev);
        if (visited) Memory_free(visited);
        if (qItems) Memory_free(qItems);
        return 0;
    }

    for (uint32_t i = 0; i < N; i++) {
        dist[i] = DIJKSTRA_INFINITY;
        prev[i] = 0xFFFFFFFFu;
        visited[i] = false;
    }

    dist[startNode] = 0.0f;
    MinQ q;
    q.items = qItems;
    q.size = 0;
    q.capacity = N * 4 + 16;
    MinQ_push(&q, startNode, 0.0f);

    while (q.size > 0) {
        uint32_t u;
        float d;
        if (!MinQ_pop(&q, &u, &d)) break;

        if (visited[u]) continue;
        visited[u] = true;

        if (u == goalNode) break;

        const DijkstraNode *node = &(*graph).nodes[u];
        for (uint32_t e = 0; e < (*node).edgeCount; e++) {
            uint32_t v = (*node).edges[e].target;
            float w = (*node).edges[e].weight;
            if (!visited[v] && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
                MinQ_push(&q, v, dist[v]);
            }
        }
    }

    if (dist[goalNode] >= DIJKSTRA_INFINITY) {
        // Unreachable
        Memory_free(dist);
        Memory_free(prev);
        Memory_free(visited);
        Memory_free(qItems);
        return 0;
    }

    if (outTotalDistance != nullptr) {
        *outTotalDistance = dist[goalNode];
    }

    // Reconstruct path backward
    uint32_t tempPath[1024];
    size_t pathLen = 0;
    uint32_t curr = goalNode;
    while (curr != 0xFFFFFFFFu && pathLen < 1024) {
        tempPath[pathLen++] = curr;
        if (curr == startNode) break;
        curr = prev[curr];
    }

    // Reverse into outPath
    size_t count = (pathLen < maxPathNodes) ? pathLen : maxPathNodes;
    for (size_t i = 0; i < count; i++) {
        outPath[i] = tempPath[pathLen - 1 - i];
    }

    Memory_free(dist);
    Memory_free(prev);
    Memory_free(visited);
    Memory_free(qItems);
    return count;
}
