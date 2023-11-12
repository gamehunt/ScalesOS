#include "cfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cfg_data* cfg_open(const char* path) {
	FILE* f = fopen(path, "r");
	if(!f) {
		return NULL;
	}

	cfg_data* cfg = malloc(sizeof(cfg_data));
	cfg->nodes = list_create();

	char line[128];
	while(fgets(line, 128, f)) {
		if(line[0] == '#' || !strlen(line)) {
			continue;
		}
	
		char name[128];
		char val[256];

		char* v = strtok(line, "=");
		int m = 0;

		if(v) {
			strncpy(name, v, 128);
		} else {
			m = 1;
			goto end;
		}

		v = strtok(NULL, "=\n");
		if(v) {
			strncpy(val, v, 256);
		} else {
			m = 1;
			goto end;
		}

end:
		if(m) {
			printf("Skipping malformed line: %s\n", line);
		} else {
			cfg_node* node = malloc(sizeof(cfg_node));
			node->name  = strdup(name);
			node->value = strdup(val);
			list_push_back(cfg->nodes, node);
		}
	}

	fclose(f);

	return cfg;
}

void cfg_free(cfg_data* cfg) {
	for(size_t i = 0; i < cfg->nodes->size; i++) {
		cfg_node* node = cfg->nodes->data[i];
		free(node->name);
		free(node->value);
		free(node);
	}
	list_free(cfg->nodes);
	free(cfg);
}

const char* cfg_get_value(cfg_data* cfg, const char* path) {
	for(size_t i = 0; i < cfg->nodes->size; i++) {
		cfg_node* node = cfg->nodes->data[i];
		if(!strcmp(node->name, path)) {
			return node->value;
		}
	}
	return NULL;
}
