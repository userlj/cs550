#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// #define N_THREADS 500

extern "C" void *
thread_func(void *vp) {
    int id = (long) vp;
    printf("This is thread %d...\n", id);
    pthread_exit(0);
}

int
main(int argc, char *argv[]) {

    assert(argc == 2);

    int n = atoi(argv[1]);

    pthread_t *threads = new pthread_t[n];

    for (int i = 0; i < n; i++) {
        int ec = pthread_create(&threads[i], 0, thread_func, (void *) long(i));
        assert(ec == 0);
    }

    delete [] threads;

    exit(0);
}