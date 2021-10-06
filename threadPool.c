//Details : Allen Bronshtein 206228751

#include "threadPool.h"

/**From idle queue, take next task and run**/
void insertNewRunnable(ThreadPool *threadPool) {
    if (threadPool != NULL) {
        Task *task = osDequeue(threadPool->idle_tasks);
        size_t i = (size_t) osDequeue(threadPool->thread_index);
        threadPool->running_threads[i] = task->thread;
        threadPool->current_running++;
        int status = pthread_create(&threadPool->running_threads[i], NULL, (void *) task->func, (void *) task->args);
        if (status != 0) {
            fprintf(stderr, "%s\n", "Error creating runnable thread");
            tpDestroy(threadPool, 1);
            exit(1);
        }
        free(task);
    }
}

/**Delete threads that died**/
void deleteDeadThreads(ThreadPool *threadPool,int mode) {
    size_t i;
    if (threadPool != NULL) {
        for (i = 0; i < threadPool->cores; i++) {
            /*Dead thread in location i*/
            if (threadPool->running_threads[i] != 0) {
                int status = pthread_kill(threadPool->running_threads[i], 0);
                if (status != RUNNING) {
                    pthread_join(threadPool->running_threads[i], NULL);
                    threadPool->running_threads[i] = 0;
                    threadPool->current_running--;
                    if(mode == ENQUEUE_INDEX) {
                        osEnqueue(threadPool->thread_index, (void *) i);
                    }
                }
            }
        }
    }
}

/**Manage the idle queue and running threads**/
void execute(ThreadPool *threadPool, int mode) {
    if (threadPool != NULL) {
        if(threadPool->shouldWaitForTasks) {
            deleteDeadThreads(threadPool, ENQUEUE_INDEX);
        }else{
            deleteDeadThreads(threadPool, 0);
        }
        if (mode == READ_TASKS) {
            if (!osIsQueueEmpty(threadPool->idle_tasks) && threadPool->current_running < threadPool->cores) {
                insertNewRunnable(threadPool);
            }
        }
    }
}

/**Main thread of executor**/
void *executorTask(void *args) {
    ThreadPool *threadPool = (ThreadPool *) args;
    if (threadPool != NULL) {
        while (!threadPool->stop) {
            execute(threadPool, READ_TASKS);
        }
        /*User requested to stop*/
        if (!threadPool->shouldWaitForTasks) {
            while (threadPool->current_running != 0) {
                execute(threadPool, !READ_TASKS);
            }
        } else { //Wait for tasks
            while (!osIsQueueEmpty(threadPool->idle_tasks)) {
                execute(threadPool, READ_TASKS);
            }
            while (threadPool->current_running != 0) {
                execute(threadPool, !READ_TASKS);
            }
        }
    }
    return NULL;
}

/**Creates threadPool**/
ThreadPool *tpCreate(int numOfThreads) {
    ThreadPool *threadPool = (ThreadPool *) calloc(THREADPOOL_NMEMB, sizeof(ThreadPool));
    if (threadPool == NULL) {
        fprintf(stderr, "%s\n", "Error allocating threadPool memory");
        exit(0);
    }
    threadPool->cores = numOfThreads;
    threadPool->current_running = 0;
    threadPool->idle_tasks = osCreateQueue();
    if (threadPool->idle_tasks == NULL) {
        fprintf(stderr, "%s\n", "Error creating idle_tasks queue");
        free(threadPool);
        exit(0);
    }
    threadPool->thread_index = osCreateQueue();
    if (threadPool->thread_index == NULL) {
        fprintf(stderr, "%s\n", "Error allocating thread_index queue");
        free(threadPool);
        osDestroyQueue(threadPool->idle_tasks);
        exit(0);
    }
    threadPool->running_threads = (pthread_t *) calloc(PTHREAD_NMEMB,sizeof(pthread_t) * numOfThreads);
    if (threadPool->running_threads == NULL) {
        fprintf(stderr, "%s\n", "Error allocating running_threads memory");
        free(threadPool);
        osDestroyQueue(threadPool->idle_tasks);
        osDestroyQueue(threadPool->thread_index);
        exit(0);
    }
    threadPool->shouldWaitForTasks = 1;
    threadPool->stop = 0;
    size_t i;
    for (i = 1; i < threadPool->cores; i++) {
        threadPool->running_threads[i] = 0;
        osEnqueue(threadPool->thread_index, (void *) i);
    }
    int status = pthread_create(&threadPool->executor_thread, NULL, executorTask, (void *) threadPool);
    if (status != 0) {
        fprintf(stderr, "%s\n", "Error creating executor thread");
        tpDestroy(threadPool, 0);
        exit(0);
    }
    return threadPool;
}

/**Destroys threadPool**/
void tpDestroy(ThreadPool *tp, int shouldWaitForTasks) {
    if (tp != NULL) {
        tp->shouldWaitForTasks = shouldWaitForTasks;
        tp->stop = 1;
        int status = pthread_join(tp->executor_thread, NULL);
        if (status != 0) {
            fprintf(stderr, "%s\n", "Error waiting for executor_thread");
        }
        while (!osIsQueueEmpty(tp->idle_tasks)) {
            Task *task = (Task *) osDequeue(tp->idle_tasks);
            free(task);
        }
        osDestroyQueue(tp->idle_tasks);
        while(!osIsQueueEmpty(tp->thread_index)){
            osDequeue(tp->thread_index);
        }
        osDestroyQueue(tp->thread_index);
        if (tp->running_threads != NULL) {
            free(tp->running_threads);
        }
        free(tp);
    }
}

/**Inserts new task to idle queue**/
int tpInsertTask(ThreadPool *tp, void (*computeFunc)(void *), void *param) {
    if (tp != NULL && computeFunc != NULL) {
        Task *task = (Task *) malloc(sizeof(Task));
        if (task == NULL) { return -1; }
        task->func = computeFunc;
        task->args = param;
        osEnqueue(tp->idle_tasks, (void *) task);
    }
    return 0;
}