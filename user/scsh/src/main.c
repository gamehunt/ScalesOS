#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE_LENGTH 4096

int try_builtin_command(const char* op, int argc, char** argv, FILE* out, FILE* in) {
	if(!out) {
		out = stdout;
	}
	if(!strcmp(op, "echo")) {
		for(int i = 0; i < argc; i++) {
			fprintf(out, "%s ", argv[i]);
		}
		fprintf(out, "\r\n");
		return 0;
	}

	return 1;
}

int execute(char* path, int argc, char** argv) {
	FILE* script = fopen(path, "r");
	if(!script) {
		fprintf(stderr, "%s: no such file\r\n", path);
		return 1;
	}
	
	char command[MAX_LINE_LENGTH];

	while(fgets(command, MAX_LINE_LENGTH, script)) {
		size_t len = strlen(command);
		if(command[len - 1] == '\n') {
			command[len - 1] = '\0';
		}

		char* start = command;
		
		while(*start && isspace(*start)) {
			start++;
		}

		if(*start == '\0' || *start == '#') {
			continue;
		}
		
		char* word = strtok(start, " ");	
		
		if(!word) {
			continue;
		}
	
		char*  op = malloc(strlen(word) + 1);
		strcpy(op, word);
		
		int    _op_argc = 0;
		char** _op_argv = 0;

		FILE*  out_pipe   = 0;
		FILE*  in_pipe    = 0;
		int    parse_pipe = 0;

		word = strtok(NULL, " ");
		while(word) {

			if(!strcmp(word, ">")) {
				parse_pipe = 1;
				word = strtok(NULL, " ");
				continue;
			}

			if(!strcmp(word, ">")) {
				parse_pipe = 2;
				word = strtok(NULL, " ");
				continue;
			}

			if(parse_pipe == 1) {
				out_pipe = fopen(word, "r");
				parse_pipe = 0;
				word = strtok(NULL, " ");
				continue;
			} else if(parse_pipe == 2) {
				in_pipe = fopen(word, "r");
				parse_pipe = 0;
				word = strtok(NULL, " ");
				continue;
			}

			_op_argc++;

			if(!_op_argv) {
				_op_argv = malloc(sizeof(char*) * 2);
			} else {
				_op_argv = realloc(_op_argv, sizeof(char*) * (_op_argc + 1));
			}

			_op_argv[_op_argc - 1] = malloc(strlen(word) + 1);
			strcpy(_op_argv[_op_argc - 1], word);

			_op_argv[_op_argc] = 0;

			word = strtok(NULL, " ");
		}

		char path[255];

		if(op[0] != '/') {
			sprintf(path, "/bin/%s", op); //TODO Path?
		} else {
			strcpy(path, op);
		}

		pid_t child = fork();

		if(!child) {
			fclose(script);
			FILE* test = fopen(path, "r");
			if(test) {
				fclose(test);
				if(out_pipe) {
					dup2(fileno(out_pipe), 1);
					dup2(fileno(out_pipe), 2);
				}
				if(in_pipe) {
					dup2(fileno(in_pipe), 0);
				}
				execve(path, _op_argv, 0);
				fprintf(stderr, "Failed to execute: %s\r\n", path);
				exit(1);
			} else {
				if(try_builtin_command(op, _op_argc, _op_argv, out_pipe, in_pipe)){
					fprintf(stderr, "No such command: %s\r\n", op);
					exit(1);
				}
				exit(0);
			}
		}

		int status;
		waitpid(child, &status, 0);

		if(status) {
			return status;
		}

		if(in_pipe) {
			fclose(in_pipe);
		}

		if(out_pipe) {
			fclose(out_pipe);
		}

		free(op);
		for(int i = 0; i < _op_argc; i++){
			if(_op_argv[i]) {
				free(_op_argv[i]);
			}
		}
		free(_op_argv);
	}

	return 0;
}

int interactive() {
	fprintf(stderr, "interactive(): UNIMPL.\r\n");
	return 0;
}

int main(int argc, char** argv) {
	if(argc < 2) {
		return interactive();
	} else {
		return execute(argv[1], argc - 2, &argv[2]);
	}
}
