#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/__internals.h>


extern int              __envc;
extern struct env_var** __env;

char* getenv(const char* env_var){
	for(int i = 0; i < __envc; i++) {
		if(!strcmp(__env[i]->name, env_var)) {
			return __env[i]->value;
		}
	}
    return 0;
}
