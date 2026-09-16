#ifndef VEXSPOKE_ALGO_TREE_SIT_H
#define VEXSPOKE_ALGO_TREE_SIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TreeSitSpan {
    uint32_t start_byte;
    uint32_t end_byte;
    uint32_t start_row;
    uint32_t start_col;
    uint32_t end_row;
    uint32_t end_col;
} TreeSitSpan;

typedef struct TreeSitNode {
    uint32_t id;
    uint16_t kind;
    const char *symbol_name;
    TreeSitSpan span;
    int32_t parent;
    int32_t first_child;
    int32_t next_sibling;
    int32_t prev_sibling;
    uint32_t child_count;
    void *payload;
} TreeSitNode;

typedef struct TreeSitTree {
    TreeSitNode *nodes;
    uint32_t node_count;
    uint32_t node_capacity;
    int32_t root;
} TreeSitTree;

typedef bool (*TreeSitVisitorFn)(const TreeSitTree *tree, int32_t node_idx, int depth, void *user_data);

bool TreeSitTree_init(uint32_t initial_capacity, TreeSitTree *tree);
void TreeSitTree_destroy(TreeSitTree *tree);
int32_t TreeSitTree_add_node(
    int32_t parent_idx,
    uint16_t kind,
    const char *symbol_name,
    TreeSitSpan span,
    TreeSitTree *tree
);
bool TreeSitTree_walk_preorder(const TreeSitTree *tree, TreeSitVisitorFn visitor, void *user_data);
bool TreeSitTree_walk_postorder(const TreeSitTree *tree, TreeSitVisitorFn visitor, void *user_data);
int32_t TreeSitTree_find_by_byte(const TreeSitTree *tree, uint32_t byte_offset);
uint32_t TreeSitTree_count_kind(const TreeSitTree *tree, uint16_t kind);

#ifdef __cplusplus
}
#endif

#endif // VEXSPOKE_ALGO_TREE_SIT_H
