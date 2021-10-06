//Details : Allen Bronshtein 206228751

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "osqueue.h"

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#define RUNNING 0
#define READ_TASKS 1
#define ENQUEUE_INDEX 1
#define PTHREAD_NMEMB 240
#define THREADPOOL_NMEMB 8

typedef struct task {
    void (*func)(void *);

    void *args;
    pthread_t thread;
} Task;

typedef struct thread_pool {
    size_t cores;
    size_t current_running;
    OSQueue *idle_tasks;
    OSQueue *thread_index;
    pthread_t *running_threads;
    pthread_t executor_thread;
    int shouldWaitForTasks;
    short stop;
} ThreadPool;

ThreadPool *tpCreate(int numOfThreads);

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param);

#endif
