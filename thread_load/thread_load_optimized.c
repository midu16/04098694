#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define THREADS 64
#define THREADS_PER_MUTEX 4
#define MUTEX_COUNT (THREADS / THREADS_PER_MUTEX)
#define LOOP_COUNT 100000
#define THREAD_SLEEP_US 100
#define OPERATIONS 100000

// Replace mutexes with spinlocks
pthread_spinlock_t spinlocks[MUTEX_COUNT];
pthread_spinlock_t mt_count;

int ops = 0;

// Dummy logging function for illustration
void timestamp_log(const char *fmt, int val) {
    printf(fmt, val);
}

void* thread_func(void* arg) {
    int index = *(int *)arg;
    free(arg);

    pthread_spinlock_t *lock = &spinlocks[index / THREADS_PER_MUTEX];

    while (1) {
        pthread_spin_lock(lock);

        volatile int temp = 0;
        for (int i = 0; i < LOOP_COUNT; i++) {
            temp += i;
        }

        pthread_spin_unlock(lock);

        if (OPERATIONS > 0) {
            pthread_spin_lock(&mt_count);
            if (ops >= OPERATIONS) {
                timestamp_log("operation finished - %d\n", ops);
                pthread_spin_unlock(&mt_count);
                exit(0);
            }
            ops++;
            pthread_spin_unlock(&mt_count);
        }

        usleep(THREAD_SLEEP_US);
    }

    return NULL;
}

int main(void) {
    pthread_t threads[THREADS];

    // Init spinlocks
    for (int i = 0; i < MUTEX_COUNT; i++) {
        if (pthread_spin_init(&spinlocks[i], PTHREAD_PROCESS_PRIVATE) != 0) {
            perror("spinlock init failed");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_spin_init(&mt_count, PTHREAD_PROCESS_PRIVATE) != 0) {
        perror("spinlock init failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < THREADS; i++) {
        int *arg = malloc(sizeof(int));
        *arg = i;
        if (pthread_create(&threads[i], NULL, thread_func, arg) != 0) {
            perror("thread create failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < MUTEX_COUNT; i++) {
        pthread_spin_destroy(&spinlocks[i]);
    }
    pthread_spin_destroy(&mt_count);

    return 0;
}
