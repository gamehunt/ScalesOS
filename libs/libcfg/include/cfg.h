#ifndef __LIB_CFG_H
#define __LIB_CFG_H

#include "types/list.h"

typedef struct _cfg_node {
	char*  name;
	char*  value;
} cfg_node;

typedef struct {
	list_t* nodes;
} cfg_data;

cfg_data* cfg_open(const char* path);
void cfg_free(cfg_data*);

const char* cfg_get_value(cfg_data* data, const char* path);

#endif
