#ifndef __K_UTIL_TYPES_TREE_H
#define __K_UTIL_TYPES_TREE_H

#ifdef __KERNEL
#include "util/types/list.h"
#else
#include "kernel/util/types/list.h"
#endif
#include <stdint.h>

struct tree;

typedef struct tree_node{
    struct tree*       tree;
    struct tree_node*  parent;
	list_t*            children;
    void*              value;
}tree_node_t;

typedef struct tree{
    tree_node_t*  root;
	list_t*       nodes;
}tree_t;

tree_t*      tree_create();
void         tree_set_root(tree_t* tree, tree_node_t* node);
tree_node_t* tree_create_node(void* value);
void         tree_free_node(tree_node_t* node);
void         tree_free(tree_t* tree);
void         tree_insert_node(tree_t* tree, tree_node_t* node, tree_node_t* parent);
void         tree_remove_node(tree_t* tree, tree_node_t* node);
uint8_t      tree_contains_node(tree_t* tree, tree_node_t* node);

#endif
