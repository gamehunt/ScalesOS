#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int    _envc;
extern char** environ;

char* getenv(const char* name){
    char** e   = environ;
	size_t len = strlen(name);
	while (*e) {
		char * t = *e;
		if (strstr(t, name) == *e) {
			if (t[len] == '=') {
				return &t[len+1];
			}
		}
		e++;
	}
	return NULL;;
}

int putenv(char* string) {
	char name[strlen(string)+1];
	strcpy(name, string);
	
	char* c = strchr(name, '=');
	
	if (!c) {
		return 1;
	}

	*c = 0;

	int s = strlen(name);

	int i;
	for (i = 0; i < (_envc - 1) && environ[i]; ++i) {
		if (!strnstr(name, environ[i], s) && environ[i][s] == '=') {
			environ[i] = string;
			return 0;
		}
	}
	/* Not found */
	if (i == _envc - 1) {
		int _new_envc = _envc * 2;
		char ** new_environ = malloc(sizeof(char*) * _new_envc);
		int j = 0;
		while (j < _new_envc && environ[j]) {
			new_environ[j] = environ[j];
			j++;
		}
		while (j < _new_envc) {
			new_environ[j] = NULL;
			j++;
		}
		_envc = _new_envc;
		environ = new_environ;
	}

	environ[i] = string;
	return 0;
}

int setenv(const char *name, const char *value, int overwrite) {
	if (!overwrite) {
		char* tmp = getenv(name);
		if (tmp) {
			return 0;
		}
	}
	char * tmp = malloc(strlen(name) + strlen(value) + 2);
	*tmp = '\0';
	strcat(tmp, name);
	strcat(tmp, "=");
	strcat(tmp, value);
	return putenv(tmp);
}

int unsetenv(const char * str) {
	int last_index = -1;
	int found_index = -1;
	int len = strlen(str);

	for (int i = 0; environ[i]; ++i) {
		if (found_index == -1 && (strstr(environ[i], str) == environ[i] && environ[i][len] == '=')) {
			found_index = i;
		}
		last_index = i;
	}

	if (found_index == -1) {
		/* not found = success */
		return 0;
	}

	if (last_index == found_index) {
		/* Was last element */
		environ[last_index] = NULL;
		return 0;
	}

	/* Was not last element, swap ordering */
	environ[found_index] = environ[last_index];
	environ[last_index] = NULL;
	return 0;
}
