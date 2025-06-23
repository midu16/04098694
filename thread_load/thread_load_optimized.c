#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sched.h>

// Number of threads
#define NUM_THREADS 64
#define LOOP_ITERATIONS 100000000

// Spinlock for shared counter
pthread_spinlock_t spinlock;

uint64_t shared_counter = 0;

void *thread_function(void *arg) {
    int cpu = (int)(intptr_t)arg;

    // Pin this thread to its own CPU core
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        fprintf(stderr, "Warning: Could not set CPU affinity for thread %d: %s\n", cpu, strerror(errno));
    }

    for (int i = 0; i < LOOP_ITERATIONS; i++) {
        pthread_spin_lock(&spinlock);
        shared_counter++;
        pthread_spin_unlock(&spinlock);
    }

    pthread_exit(NULL);
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    int rc;

    if (pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE) != 0) {
        perror("pthread_spin_init");
        exit(EXIT_FAILURE);
    }

    for (intptr_t i = 0; i < NUM_THREADS; i++) {
        rc = pthread_create(&threads[i], NULL, thread_function, (void *)i);
        if (rc != 0) {
            fprintf(stderr, "Error creating thread %ld: %s\n", i, strerror(rc));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_spin_destroy(&spinlock);

    printf("Final counter value: %lu\n", shared_counter);

    return 0;
}
