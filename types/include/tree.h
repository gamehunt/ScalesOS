#ifndef __TYPES_TREE_H
#define __TYPES_TREE_H

#include <stdint.h>

struct list;
struct tree;

typedef struct tree_node{
    struct tree*       tree;
    struct tree_node*  parent;
	struct list*       children;
    void*              value;
} tree_node_t;

typedef struct tree{
    tree_node_t*  root;
	struct list*  nodes;
} tree_t;

tree_t*      tree_create();
void         tree_set_root(tree_t* tree, tree_node_t* node);
tree_node_t* tree_create_node(void* value);
void         tree_free_node(tree_node_t* node);
void         tree_free(tree_t* tree);
void         tree_insert_node(tree_t* tree, tree_node_t* node, tree_node_t* parent);
void         tree_remove_node(tree_t* tree, tree_node_t* node);
uint8_t      tree_contains_node(tree_t* tree, tree_node_t* node);

#endif
