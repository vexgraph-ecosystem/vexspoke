#ifndef ALGO_DIJKSTRA_H
#define ALGO_DIJKSTRA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// algo/dijkstra.h — Shortest-Path Graph Solver.
//
// Solves single-source shortest paths on weighted directed/undirected graphs
// using a priority queue (MinHeap). Used for routing, spatial navigation,
// and audio node dependency graphs.

#define DIJKSTRA_INFINITY 1e30f

typedef struct DijkstraEdge {
    uint32_t target;
    float    weight;
} DijkstraEdge;

typedef struct DijkstraNode {
    uint32_t      edgeCount;
    uint32_t      edgeCapacity;
    DijkstraEdge *edges;
} DijkstraNode;

typedef struct DijkstraGraph {
    uint32_t      nodeCount;
    DijkstraNode *nodes;
} DijkstraGraph;

DijkstraGraph *Dijkstra_create(uint32_t nodeCount);
void           Dijkstra_free(DijkstraGraph *graph);

bool Dijkstra_addEdge(DijkstraGraph *graph, uint32_t from, uint32_t to, float weight);
bool Dijkstra_addBiEdge(DijkstraGraph *graph, uint32_t a, uint32_t b, float weight);

// Solve shortest path from start to goal.
// Dest-last order: writes node indices into outPath and path length into outTotalDist.
// Returns number of nodes in the path (0 if unreachable).
size_t Dijkstra_shortestPath(const DijkstraGraph *graph,
                             uint32_t             startNode,
                             uint32_t             goalNode,
                             uint32_t            *outPath,
                             size_t               maxPathNodes,
                             float               *outTotalDistance);

#endif
