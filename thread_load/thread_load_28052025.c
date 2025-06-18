#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <getopt.h>
#include <sched.h>
#include <errno.h>

int THREAD_BATCH = 100;
int MAX_THREADS;
int THREADS_PER_MUTEX = 20;
int LOOP_COUNT = 1000;
int THREAD_SLEEP_US = 1000;
int MAX_MUTEXES;
int OPERATIONS = 0;
int SINGLE_PROCESS = 0;
volatile int ops = 0;

pthread_mutex_t *mutexes;
pthread_mutex_t mt_count;

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
    printf("  -i <THREAD_BATCH_INTERVAL_SEC>   Sleep time between thread batches (default: 30)\n");
    printf("  -b <THREAD_BATCH>                Number of threads per batch (default: 100)\n");
    printf("  -m <MAX_THREADS>                 Total number of threads (default: 2 * cores)\n");
    printf("  -t <THREADS_PER_MUTEX>           Number of threads per mutex (default: 20)\n");
    printf("  -l <LOOP_COUNT>                  Number of CPU loop iterations inside thread (default: 1000)\n");
    printf("  -d <THREAD_SLEEP_US>             Sleep time inside thread function in microseconds (default: 1000)\n");
    printf("  -o <OPERATIONS>                  Operations before termination (default: 0)\n");
    printf("  -s                               Single-process mode (no forks)\n");
    printf("  -h                               Show this help message\n");
}

void* thread_func(void* arg) {
    int index = *(int *)arg;
    pthread_mutex_t *mutex = &mutexes[index / THREADS_PER_MUTEX];
    free(arg);

    struct timespec ts = {0, THREAD_SLEEP_US * 1000};

    while (1) {
        pthread_mutex_lock(mutex);

        volatile int temp = 0;
        for (int i = 0; i < LOOP_COUNT; i++) {
            temp += i;
        }

        pthread_mutex_unlock(mutex);

        if (OPERATIONS > 0) {
            int local_ops;
            pthread_mutex_lock(&mt_count);
            if (ops >= OPERATIONS) {
                pthread_mutex_unlock(&mt_count);
                timestamp_log("operation finished - %d", ops);
                exit(0);
            }
            local_ops = ++ops;
            pthread_mutex_unlock(&mt_count);
        }

        nanosleep(&ts, NULL);
    }
    return NULL;
}

void create_threads(int start_index, int max_cores) {
    pthread_t threads[THREAD_BATCH];

    for (int i = 0; i < THREAD_BATCH; i++) {
        int *arg = malloc(sizeof(*arg));
        if (!arg) {
            perror("malloc failed");
            exit(1);
        }
        *arg = start_index + i;

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET((*arg) % max_cores, &cpuset);

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);

        if (pthread_create(&threads[i], &attr, thread_func, arg) != 0) {
            perror("pthread_create failed");
            exit(1);
        }

        pthread_attr_destroy(&attr);
    }

    timestamp_log("[Thread Init] Created %d threads, batch starting at %d", THREAD_BATCH, start_index);
}

int main(int argc, char *argv[]) {
    int thread_batch_interval_sec = 30;
    int opt;

    int max_cores = sysconf(_SC_NPROCESSORS_ONLN);
    MAX_THREADS = max_cores * 2;

    while ((opt = getopt(argc, argv, "i:b:m:t:l:d:o:sh")) != -1) {
        switch (opt) {
            case 'i': thread_batch_interval_sec = atoi(optarg); break;
            case 'b': THREAD_BATCH = atoi(optarg); break;
            case 'm': MAX_THREADS = atoi(optarg); break;
            case 't': THREADS_PER_MUTEX = atoi(optarg); break;
            case 'l': LOOP_COUNT = atoi(optarg); break;
            case 'd': THREAD_SLEEP_US = atoi(optarg); break;
            case 'o': OPERATIONS = atoi(optarg); break;
            case 's': SINGLE_PROCESS = 1; break;
            case 'h': print_usage(argv[0]); exit(0);
            default: print_usage(argv[0]); exit(1);
        }
    }

    if (MAX_THREADS % THREADS_PER_MUTEX != 0) {
        fprintf(stderr, "MAX_THREADS must be a multiple of THREADS_PER_MUTEX.\n");
        exit(1);
    }

    MAX_MUTEXES = MAX_THREADS / THREADS_PER_MUTEX;
    mutexes = malloc(sizeof(pthread_mutex_t) * MAX_MUTEXES);
    if (!mutexes) {
        perror("malloc failed for mutexes");
        exit(1);
    }

    for (int i = 0; i < MAX_MUTEXES; i++) {
        pthread_mutex_init(&mutexes[i], NULL);
    }
    pthread_mutex_init(&mt_count, NULL);

    timestamp_log("[Config] THREAD_BATCH=%d, MAX_THREADS=%d, THREADS_PER_MUTEX=%d, LOOP_COUNT=%d, THREAD_SLEEP_US=%d, OPERATIONS=%d, SINGLE_PROCESS=%d", 
        THREAD_BATCH, MAX_THREADS, THREADS_PER_MUTEX, LOOP_COUNT, THREAD_SLEEP_US, OPERATIONS, SINGLE_PROCESS);

    int created = 0;

    if (SINGLE_PROCESS) {
        while (created < MAX_THREADS) {
            create_threads(created, max_cores);
            created += THREAD_BATCH;
            if (created < MAX_THREADS) {
                sleep(thread_batch_interval_sec);
            }
        }
        while (1) sleep(6000);
    } else {
        while (created < MAX_THREADS) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                exit(1);
            } else if (pid == 0) {
                timestamp_log("[Child] PID %d started", getpid());
                create_threads(created, max_cores);
                while (1) sleep(6000);
            } else {
                timestamp_log("[Parent] Forked child PID %d", pid);
                created += THREAD_BATCH;
                if (created < MAX_THREADS) {
                    sleep(thread_batch_interval_sec);
                }
            }
        }

        int status;
        pid_t wpid;
        while ((wpid = wait(&status)) > 0) {
            sleep(1);
        }
    }

    free(mutexes);
    return 0;
}