#include "util/types/list.h"
#include <mem/heap.h>
#include <string.h>
#include <util/types/tree.h>

tree_t* tree_create() {
    tree_t* tree = k_calloc(1, sizeof(tree_t));
	tree->nodes = list_create();
	return tree;
}

void tree_set_root(tree_t* tree, tree_node_t* node) {
    tree_insert_node(tree, node, 0);
    tree->root = node;
}

tree_node_t* tree_create_node(void* value) {
    tree_node_t* tree_node = k_calloc(1, sizeof(tree_node_t));
    tree_node->value = value;
	tree_node->children = list_create();
    return tree_node;
}

void tree_free_node(tree_node_t* node) {
	list_free(node->children);
    k_free(node);
}

void tree_free(tree_t* tree) {
    for (uint32_t i = 0; i < tree->nodes->size; i++) {
        tree_free_node((tree_node_t*) tree->nodes[i].data);
    }
    list_free(tree->nodes);
    k_free(tree);
}

void tree_insert_node(tree_t* tree, tree_node_t* node, tree_node_t* parent) {
    if (tree_contains_node(tree, node)) {
        return;
    }
	list_push_back(tree->nodes, node);
    node->tree = tree;
    if (parent) {
        node->parent = parent;
		list_push_back(parent->children, node);
    }
}

void tree_remove_node(tree_t* tree, tree_node_t* node) {
	if(!tree_contains_node(tree, node)) {
		return;
	}
	tree_node_t* parent = node->parent;
	if(parent) {
		list_delete_element(parent->children, node);
	}
	list_delete_element(tree->nodes, node);
}

uint8_t tree_contains_node(tree_t* tree, tree_node_t* node) {
    return node->tree == tree;
}
