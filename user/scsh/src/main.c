#include "errno.h"
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/tty.h>
#include <dirent.h>
#include <pwd.h>

#define MAX_LINE_LENGTH 4096

const char* pwd  = "";
const char* user = "";

uint8_t is_interactive = 0;
pid_t   my_pid = 0;

uint8_t interrupted = 0;

void sigint_handler(int sig) {
	fprintf(stderr, "\nSIGINT\n");
	interrupted = 1;
}

void calc(const char* expr, FILE* out) {
	pid_t child = fork();
	if(!child) {
		double x = 0;
		while(x < 25) {
			fprintf(out, "sqrt(%f) = %f\n", x, sqrt(x));
			x += 0.5;
		}
		exit(0);
	}
	int status;
	waitpid(child, &status, 0);
}

int try_builtin_command(const char* op, int argc, char** argv, FILE* out, FILE* in) {
	if(!out) {
		out = stdout;
	}

	if(!in) {
		in = stdin;
	}

	if(op[0] == '&') {
		char expr[4096];
		strncat(expr, op, sizeof(expr));
		for(int i = 0; i < argc; i++) {
			strncat(expr, argv[i], sizeof(expr) - strlen(expr) - 1);
		}
		calc(op, out);
		return 0;
	}

	if(!strcmp(op, "echo")) {
		for(int i = 0; i < argc; i++) {
			fprintf(out, "%s ", argv[i]);
		}
		fprintf(out, "\n");
		return 0;
	} else if(!strcmp(op, "cd")) {
		if(argc < 1) {
			fprintf(out, "Usage: cd <directory>\n");
			return 1;
		}
		int r = chdir(argv[0]);
		if(r < 0) {
			if(errno == EINVAL) {
				fprintf(out, "No such directory: %s\n", argv[0]);
			} else if(errno == ENOTDIR) {
				fprintf(out, "Not a directory: %s\n", argv[0]);
			} else {
				fprintf(out, "Unknown error occured: %ld\n", errno);
			}
			return 1;
		}
		pwd = get_current_dir_name();
		return 0;
	} else if(!strcmp(op, "pwd")) {
		fprintf(out, "%s\n", pwd);
		return 0;
	}

	return -1;
}

int execute_line(char* line) {
	
	char* word = strtok(line, " ");
	char* op   = strdup(word);
	int    _op_argc = 0;
	char** _op_argv = 0;

	FILE*  out_pipe   = 0;
	FILE*  in_pipe    = 0;
	int    parse_pipe = 0;

	int run_in_bg = 0;

	word = strtok(NULL, " ");

	while(word) {
		if(!strcmp(word, ">")) {
			parse_pipe = 1;
			word = strtok(NULL, " ");
			continue;
		}

		if(!strcmp(word, "<")) {
			parse_pipe = 2;
			word = strtok(NULL, " ");
			continue;
		}

		if(parse_pipe == 1) {
			out_pipe = fopen(word, "a+");
			parse_pipe = 0;
			word = strtok(NULL, " ");
			continue;
		} else if(parse_pipe == 2) {
			in_pipe = fopen(word, "r");
			parse_pipe = 0;
			word = strtok(NULL, " ");
			continue;
		}

		char* next_word = strtok(NULL, " ");

		if(!next_word) {
			if(!strcmp(word, "&")) {
				run_in_bg = 1;
				break;
			}
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

		word = next_word;
	}

	char path[255];

	if(op[0] != '/') {
		snprintf(path, 255, "/bin/%s", op); //TODO Path?
	} else {
		strncpy(path, op, 255);
	}

	int status = 0;

	FILE* test = fopen(path, "r");
	if(test) {
		fclose(test);
		pid_t child = fork();
		if(!child) {
			if(out_pipe) {
				dup2(fileno(out_pipe), 1);
				dup2(fileno(out_pipe), 2);
			}
			if(in_pipe) {
				dup2(fileno(in_pipe), 0);
			}
			execve(path, _op_argv, 0);
			fprintf(stderr, "Failed to execute: %s\n", path);
			exit(1);
		}
		if(!run_in_bg) {
			tcsetpgrp(STDIN_FILENO, child);
			waitpid(child, &status, 0);
			tcsetpgrp(STDIN_FILENO, my_pid);
		} else if(is_interactive) {
			printf("In background: %d\n", child);
		}
	} else {
		status = try_builtin_command(op, _op_argc, _op_argv, out_pipe, in_pipe);
		if(status < 0){
			fprintf(stderr, "No such command: %s\n", op);
		}
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

	return status;
}

int execute_script(char* buffer, int argc, char** argv) {
	interrupted = 0;

	char*  line  = strtok(buffer, "\n;");
	char** lines = malloc(sizeof(char*));
	int lc = 0;
	while(line) {
		char* start = line;	

		while(*start && isspace(*start)) {
			start++;
		}

		if(*start == '\0' || *start == '#') {
			line = strtok(NULL, "\n;");
			continue;
		}

		lines[lc] = malloc(strlen(line) + 1);
		strcpy(lines[lc], line);

		lc++;
		lines = realloc(lines, (lc + 1) * sizeof(char*));

		line = strtok(NULL, "\n;");
	}

	for(int i = 0; i < lc; i++) {
		int status = execute_line(lines[i]);;
		if(status || interrupted) {
			for(int j = i + 1; j < lc; j++) {
				free(lines[j]);
			}
			free(lines);
			if(status) {
				fprintf(stderr, "Error: %s returned non-zero status: %d\n", lines[i], status);
			}
			return status;
		}
	}

	free(lines);

	return 0;
}

int execute(char* path, int argc, char** argv) {
	FILE* script = fopen(path, "r");
	if(!script) {
		fprintf(stderr, "%s: no such file\n", path);
		return 1;
	}
	
	size_t size = fseek(script, 0, SEEK_END);
	fseek(script, 0, SEEK_SET);

	char* buffer = malloc(size);
	fread(buffer, 1, size, script);
	fclose(script);

	return execute_script(buffer, argc, argv);
}

void usage() {

}

int interactive() {
	is_interactive = 1;
	tcsetpgrp(STDIN_FILENO, my_pid);
	signal(SIGINT, &sigint_handler);
	while(1) {
		printf("%s:%s > ", user, pwd);
		fflush(stdout);
		char line[MAX_LINE_LENGTH];
		fgets(line, MAX_LINE_LENGTH, stdin);
		if(interrupted) {
			interrupted = 0;
			printf("\n");
			continue;
		}
		if(line[0] == '\n') {
			continue;
		}
		execute_script(line, NULL, NULL);
	}
	return 0;
}

int main(int argc, char** argv) {
	
	pwd  = get_current_dir_name();
	
	struct passwd* usr = getpwuid(getuid());

	if(usr) {
		user = strdup(usr->pw_name);
	}

	my_pid = getpid();

	if(argc < 2) {
		return interactive();
	} else if(argv[1][0] != '-'){
		return execute(argv[1], argc - 2, &argv[2]);
	} else if(argv[1][1] == 'c') {
		if(argc >= 3) {
			return execute_script(argv[2], argc - 3, &argv[3]);
		} else {
			usage();
		}
	}
	
	return 0;
}
