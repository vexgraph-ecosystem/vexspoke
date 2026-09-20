#include "algo/tree_sit.h"

#include <stdlib.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: TreeSit
 * ============================================================================
 * Relational AST tree-sitter graph structure: a flat, dynamically-grown node
 * array where each node carries parent/sibling/first-child links plus a
 * byte/row-col span, enabling pre-order, post-order, and span-based queries
 * without pointer chasing. Exists because language grammars (R3) and editors
 * (R4/R5) need a stable, allocation-friendly syntax tree that hot-reloadable
 * modules can build and walk. Memory: the node array doubles exponentially
 * (the Dynamic Scalability & Anti-Hardcoding Law); the root is a signed index
 * (-1 when empty). Lifetime: the TreeSitTree_init/TreeSitTree_destroy pair;
 * symbol names are borrowed, never owned.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: TreeSit (algo/tree_sit.c)
 * LEVEL: L3 — Structural Subsystem (Syntax & Relational AST Traversal)
 * ============================================================================
 * Relational AST tree-sitter graph structure with pre-order, post-order, and
 * span-based query capabilities.
 *
 * STRUCT FIELDS:
 *   - TreeSitSpan: byte and row/col coordinates
 *   - TreeSitNode: parent, sibling, first_child links, payload
 *   - TreeSitTree: dynamic node array and root pointer
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Tree Lifecycle:
 *   - TreeSitTree_init(initial_capacity, tree)
 *   - TreeSitTree_destroy(tree)
 *   - TreeSitTree_add_node(parent_idx, kind, symbol_name, span, tree)
 * Traversal & Search:
 *   - TreeSitTree_walk_preorder(tree, visitor, user_data)
 *   - TreeSitTree_walk_postorder(tree, visitor, user_data)
 *   - TreeSitTree_find_by_byte(tree, byte_offset)
 *   - TreeSitTree_count_kind(tree, kind)
 * ============================================================================
 */

;;INTENTION("AST node graph representation and hierarchy traversal")

bool TreeSitTree_init(uint32_t initial_capacity, TreeSitTree *tree) {
    if (!tree) {
        return false;
    }
    uint32_t cap = (initial_capacity < 16) ? 16 : initial_capacity;
    (*tree).nodes = (TreeSitNode*) malloc(cap * sizeof(TreeSitNode));
    if (!(*tree).nodes) {
        (*tree).node_count = 0;
        (*tree).node_capacity = 0;
        (*tree).root = -1;
        return false;
    }
    (*tree).node_count = 0;
    (*tree).node_capacity = cap;
    (*tree).root = -1;
    return true;
}

void TreeSitTree_destroy(TreeSitTree *tree) {
    if (!tree) {
        return;
    }
    if ((*tree).nodes) {
        free((*tree).nodes);
        (*tree).nodes = NULL;
    }
    (*tree).node_count = 0;
    (*tree).node_capacity = 0;
    (*tree).root = -1;
}

int32_t TreeSitTree_add_node(
    int32_t parent_idx,
    uint16_t kind,
    const char *symbol_name,
    TreeSitSpan span,
    TreeSitTree *tree
) {
    if (!tree) {
        return -1;
    }

    if ((*tree).node_count >= (*tree).node_capacity) {
        uint32_t new_cap = (*tree).node_capacity * 2;
        TreeSitNode *new_nodes = (TreeSitNode*) realloc((*tree).nodes, new_cap * sizeof(TreeSitNode));
        if (!new_nodes) {
            return -1;
        }
        (*tree).nodes = new_nodes;
        (*tree).node_capacity = new_cap;
    }

    int32_t new_idx = (int32_t) (*tree).node_count++;
    TreeSitNode *node = &(*tree).nodes[new_idx];

    (*node).id = (uint32_t) new_idx;
    (*node).kind = kind;
    (*node).symbol_name = symbol_name;
    (*node).span = span;
    (*node).parent = parent_idx;
    (*node).first_child = -1;
    (*node).next_sibling = -1;
    (*node).prev_sibling = -1;
    (*node).child_count = 0;
    (*node).payload = NULL;

    if (parent_idx < 0) {
        if ((*tree).root < 0) {
            (*tree).root = new_idx;
        }
    } else if ((uint32_t) parent_idx < (*tree).node_count - 1) {
        TreeSitNode *parent = &(*tree).nodes[parent_idx];
        if ((*parent).first_child < 0) {
            (*parent).first_child = new_idx;
        } else {
            int32_t curr = (*parent).first_child;
            while ((*tree).nodes[curr].next_sibling >= 0) {
                curr = (*tree).nodes[curr].next_sibling;
            }
            (*tree).nodes[curr].next_sibling = new_idx;
            (*node).prev_sibling = curr;
        }
        (*parent).child_count++;
    }

    return new_idx;
}

static bool walk_preorder_rec(
    const TreeSitTree *tree,
    int32_t node_idx,
    int depth,
    TreeSitVisitorFn visitor,
    void *user_data
) {
    if (node_idx < 0 || (uint32_t) node_idx >= (*tree).node_count) {
        return true;
    }

    if (!visitor(tree, node_idx, depth, user_data)) {
        return false;
    }

    int32_t child = (*tree).nodes[node_idx].first_child;
    while (child >= 0) {
        if (!walk_preorder_rec(tree, child, depth + 1, visitor, user_data)) {
            return false;
        }
        child = (*tree).nodes[child].next_sibling;
    }
    return true;
}

bool TreeSitTree_walk_preorder(const TreeSitTree *tree, TreeSitVisitorFn visitor, void *user_data) {
    if (!tree || !visitor || (*tree).root < 0) {
        return false;
    }
    return walk_preorder_rec(tree, (*tree).root, 0, visitor, user_data);
}

static bool walk_postorder_rec(
    const TreeSitTree *tree,
    int32_t node_idx,
    int depth,
    TreeSitVisitorFn visitor,
    void *user_data
) {
    if (node_idx < 0 || (uint32_t) node_idx >= (*tree).node_count) {
        return true;
    }

    int32_t child = (*tree).nodes[node_idx].first_child;
    while (child >= 0) {
        if (!walk_postorder_rec(tree, child, depth + 1, visitor, user_data)) {
            return false;
        }
        child = (*tree).nodes[child].next_sibling;
    }

    return visitor(tree, node_idx, depth, user_data);
}

bool TreeSitTree_walk_postorder(const TreeSitTree *tree, TreeSitVisitorFn visitor, void *user_data) {
    if (!tree || !visitor || (*tree).root < 0) {
        return false;
    }
    return walk_postorder_rec(tree, (*tree).root, 0, visitor, user_data);
}

int32_t TreeSitTree_find_by_byte(const TreeSitTree *tree, uint32_t byte_offset) {
    if (!tree || (*tree).root < 0) {
        return -1;
    }

    int32_t curr = (*tree).root;
    int32_t best_match = -1;

    while (curr >= 0) {
        const TreeSitNode *node = &(*tree).nodes[curr];
        if (byte_offset >= (*node).span.start_byte && byte_offset <= (*node).span.end_byte) {
            best_match = curr;
            curr = (*node).first_child; // Drill down into finer-grained child
        } else {
            curr = (*node).next_sibling;
        }
    }

    return best_match;
}

uint32_t TreeSitTree_count_kind(const TreeSitTree *tree, uint16_t kind) {
    if (!tree) {
        return 0;
    }
    uint32_t count = 0;
    for (uint32_t i = 0; i < (*tree).node_count; i++) {
        if ((*tree).nodes[i].kind == kind) {
            count++;
        }
    }
    return count;
}
