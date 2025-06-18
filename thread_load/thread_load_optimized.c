#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <getopt.h>

int THREAD_BATCH = 100;
int MAX_THREADS = 20000;
int THREADS_PER_MUTEX = 20;
int LOOP_COUNT = 1000;
int THREAD_SLEEP_US = 0; // Optimized: default is no sleep
int MAX_MUTEXES;
int OPERATIONS = 0;
volatile int ops = 0;

pid_t pid, wpid;
int status = 0;

pthread_mutex_t *mutexes;

void timestamp_log(const char *format, ...) {
    va_list args;
    va_start(args, format);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    printf("[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -i <THREAD_BATCH_INTERVAL_SEC>   Sleep time between thread batches (default: 0)\n");
    printf("  -b <THREAD_BATCH>                Number of threads per batch (default: 100)\n");
    printf("  -m <MAX_THREADS>                 Total number of threads (default: 20000)\n");
    printf("  -p <THREADS_PER_MUTEX>           Threads sharing a mutex (default: 20)\n");
    printf("  -l <LOOP_COUNT>                  Loop iterations per thread (default: 1000)\n");
    printf("  -s <THREAD_SLEEP_US>             Sleep microseconds per loop (default: 0)\n");
}

void *thread_func(void *arg) {
    int thread_id = *(int *)arg;
    int mutex_index = thread_id / THREADS_PER_MUTEX;

    for (int j = 0; j < LOOP_COUNT; j++) {
        pthread_mutex_lock(&mutexes[mutex_index]);

        // Simulate work instead of sleep
        volatile int dummy = 0;
        for (int k = 0; k < 1000; k++) {
            dummy += k;
        }

        pthread_mutex_unlock(&mutexes[mutex_index]);

        // Optional short sleep (configurable)
        if (THREAD_SLEEP_US > 0)
            usleep(THREAD_SLEEP_US);
    }

    __sync_add_and_fetch(&ops, 1);
    return NULL;
}

int main(int argc, char *argv[]) {
    int opt;
    int batch_interval_sec = 0;

    while ((opt = getopt(argc, argv, "i:b:m:p:l:s:h")) != -1) {
        switch (opt) {
            case 'i': batch_interval_sec = atoi(optarg); break;
            case 'b': THREAD_BATCH = atoi(optarg); break;
            case 'm': MAX_THREADS = atoi(optarg); break;
            case 'p': THREADS_PER_MUTEX = atoi(optarg); break;
            case 'l': LOOP_COUNT = atoi(optarg); break;
            case 's': THREAD_SLEEP_US = atoi(optarg); break;
            case 'h':
            default: print_usage(argv[0]); exit(EXIT_FAILURE);
        }
    }

    MAX_MUTEXES = MAX_THREADS / THREADS_PER_MUTEX;
    mutexes = malloc(MAX_MUTEXES * sizeof(pthread_mutex_t));
    for (int i = 0; i < MAX_MUTEXES; i++) {
        pthread_mutex_init(&mutexes[i], NULL);
    }

    pthread_t *threads = malloc(MAX_THREADS * sizeof(pthread_t));
    int *thread_ids = malloc(MAX_THREADS * sizeof(int));

    for (int i = 0; i < MAX_THREADS; i += THREAD_BATCH) {
        int batch_end = i + THREAD_BATCH;
        if (batch_end > MAX_THREADS) batch_end = MAX_THREADS;

        for (int j = i; j < batch_end; j++) {
            thread_ids[j] = j;
            pthread_create(&threads[j], NULL, thread_func, &thread_ids[j]);
        }

        if (batch_interval_sec > 0)
            sleep(batch_interval_sec);
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    timestamp_log("All threads completed. Total ops: %d", ops);

    free(mutexes);
    free(threads);
    free(thread_ids);
    return 0;
}
