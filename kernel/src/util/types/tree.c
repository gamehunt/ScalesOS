#include <mem/heap.h>
#include <string.h>
#include <util/types/tree.h>

tree_t *tree_create() {
    return k_calloc(1, sizeof(tree_t));
}

void tree_set_root(tree_t *tree, tree_node_t *node) {
    tree_insert_node(tree, node, 0);
    tree->root = node;
}

tree_node_t *tree_create_node(void *value) {
    tree_node_t *tree_node = k_calloc(1, sizeof(tree_node_t));
    tree_node->value = value;
    return tree_node;
}

void tree_free_node(tree_node_t *node) {
    k_free(node->childs);
    k_free(node);
}

void tree_free(tree_t *tree) {
    for (uint32_t i = 0; i < tree->node_count; i++) {
        tree_free_node(tree->nodes[i]);
    }
    k_free(tree->nodes);
    k_free(tree);
}

void tree_insert_node(tree_t *tree, tree_node_t *node, tree_node_t *parent) {
    if (tree_contains_node(tree, node)) {
        return;
    }
    EXTEND(tree->nodes, tree->node_count, sizeof(tree_node_t *))
    tree->nodes[tree->node_count - 1] = node;
    node->tree = tree;
    if (parent) {
        node->parent = parent;
        EXTEND(parent->childs, parent->child_count, sizeof(tree_node_t *))
        parent->childs[parent->child_count - 1] = node;
    }
}

void tree_remove_node(tree_t *tree, tree_node_t *node) {
    uint32_t idx = 0;
    uint8_t found = 0;
    for (uint32_t i = 0; i < tree->node_count; i++) {
        if (tree->nodes[i] == node) {
            tree->nodes[i] = 0;
            idx = i;
            found = 1;
            break;
        }
    }
    if (found) {
        tree->node_count--;
        if (tree->node_count > 0) {
            memmove(tree->nodes, tree->nodes + idx, tree->node_count + 1 - idx);
            tree->nodes = k_realloc(tree->nodes, tree->node_count);
        } else {
            k_free(tree->nodes);
        }
    }
}

uint8_t tree_contains_node(tree_t *tree, tree_node_t *node) {
    return node->tree == tree;
}