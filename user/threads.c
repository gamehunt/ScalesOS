#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

__thread int test = 0;

void  wait_thread (void);
void* thread_func (void*);

int main (int argc, char *argv[], char *envp[]) {
    pthread_t thread;
	test = 123;
    if (pthread_create(&thread,NULL,thread_func,NULL)) return EXIT_FAILURE;
    for (unsigned int i = 0; i < 20; i++) {
        puts("a");
        wait_thread();
    }
    if (pthread_join(thread,NULL)) return EXIT_FAILURE;
	printf("[MAIN] My test is %d\n", test);
    return EXIT_SUCCESS;
}

void wait_thread (void) {
    time_t start_time = time(NULL);
    while(time(NULL) == start_time) {}
}

void* thread_func (void* vptr_args) {
	test = 321;
    for (unsigned int i = 0; i < 20; i++) {
        puts("b\n");
        wait_thread();
    }
	printf("[THREAD] My test is %d\n", test);
    return NULL;
}
