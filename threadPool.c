#include <stdio.h>
#include "threadPool.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define ERROR "Error in system call"
#define ERROR_SIZE strlen(ERROR)

void error(){
    write(STDERR_FILENO, ERROR, ERROR_SIZE);
}

void freeTp(ThreadPool* tp) {
    while (!osIsQueueEmpty(tp->tasksQueue)) {
        struct Task* task = osDequeue(tp->tasksQueue);
        if (task != NULL) {
            free(task);
        }
    }
    if (tp->threads != NULL) {
        free(tp->threads);
    }
    osDestroyQueue(tp->tasksQueue);

    pthread_mutex_destroy(&tp->taskLock);
    pthread_mutex_destroy(&tp->countMutex);
    pthread_cond_destroy(&tp->threadsCond);
    free(tp);
}

task* getNextTask(ThreadPool* tp){
    if (pthread_mutex_lock(&tp->taskLock) != 0) {
        error();
        freeTp(tp);
        exit(EXIT_FAILURE);
    }
    task*  nextTask = osDequeue(tp->tasksQueue);
    if (pthread_mutex_unlock(&tp->taskLock) != 0) {
        error();
        freeTp(tp);
        exit(EXIT_FAILURE);
    }
    return  nextTask;
}

// assigning a thread to execute a task.
void* assignThread(void * th) {

    ThreadPool* threadPool = (ThreadPool *) th;
    if(pthread_mutex_trylock(&threadPool->countMutex) != 0) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    // increase count;
    threadPool->numAlive++;
    if (pthread_mutex_unlock(&threadPool->countMutex) != 0) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }

    while (threadPool->shouldWork){

        if (osIsQueueEmpty(threadPool->tasksQueue)){
            //tasks are empty and we dont need to wait, exit.
            if (!threadPool->shouldWaitToFinish) break;
            if (pthread_mutex_lock(&threadPool->taskLock) != 0) {
                error();
                freeTp(threadPool);
                exit(EXIT_FAILURE);
            }
            // wait for additional tasks.
            if (pthread_cond_wait(&threadPool->threadsCond, &threadPool->taskLock) != 0){
                error();
                freeTp(threadPool);
                exit(EXIT_FAILURE);
            }
            if (pthread_mutex_unlock(&threadPool->taskLock) != 0){
                error();
                freeTp(threadPool);
                exit(EXIT_FAILURE);
            }
        }
        if (pthread_mutex_lock(&threadPool->countMutex) != 0) {
            error();
            freeTp(threadPool);
            exit(EXIT_FAILURE);
        }
        // one more active thread.
        threadPool->numActive++;
        if (pthread_mutex_unlock(&threadPool->countMutex) != 0){
            error();
            freeTp(threadPool);
            exit(EXIT_FAILURE);
        }
        // get the next task and execute.
        task * nextTask = getNextTask(threadPool);
        if (nextTask == NULL)
            return NULL;
        nextTask->function(nextTask->arg);
        free(nextTask);
    }
    if (!threadPool->shouldWork){
        if (pthread_mutex_lock(&threadPool->countMutex) != 0){
            error();
            freeTp(threadPool);
            return NULL;
        }
        threadPool->numActive--;
        // all thread are idle(maybe some are block), so we signal to realease from the block.
        if (threadPool->numActive < 1) {
            if (pthread_cond_signal(&threadPool->threadsCond) != 0){
                error();
                freeTp(threadPool);
                return  NULL;
            }
        }
        if (pthread_mutex_unlock(&threadPool->countMutex) != 0){
            error();
            freeTp(threadPool);
            return  NULL;
        }
    }

}

int initPthreadStuff(ThreadPool* th) {
    if (pthread_mutex_init(&th->countMutex, NULL) != 0) {
        error();
        return 0;
    }
    if (pthread_mutex_init(&th->taskLock, NULL) != 0){
        error();
        if (pthread_mutex_destroy(&th->countMutex) != 0){
            error();
        }
        return 0;
    }

    if (pthread_cond_init(&th->threadsCond, NULL) != 0){
        error();
        if (pthread_mutex_destroy(&th->taskLock) != 0)
            error();
        if (pthread_mutex_destroy(&th->countMutex) != 0)
            error();
        return 0;
    }
    return 1;
}


int createThreadsArray(ThreadPool* tp, int numOfThreads){
    tp->threads = (pthread_t*)malloc(numOfThreads * sizeof(pthread_t));
    if (tp->threads == NULL) {
        error();
        freeTp(tp);
        exit(EXIT_FAILURE);
    }
    int i;
    for ( i = 0; i < numOfThreads; i++){
        if (pthread_create(&tp->threads[i], NULL, (void*)assignThread, tp) != 0){
            error();
            freeTp(tp);
            exit(EXIT_FAILURE);
        }
    }
    return 1;
}

ThreadPool* tpCreate(int numOfThreads) {
    if (numOfThreads < 1) {
        exit(0);
    }
    ThreadPool *threadPool;
    // allocate mem to the th_pool.
    threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
    // error in allocation.
    if (threadPool == NULL) {
        error();
        free(threadPool);
        exit(EXIT_FAILURE);
    }
    threadPool->shouldWork = true;
    threadPool->shouldWaitToFinish = false;
    threadPool->numActive = 0;
    threadPool->numAlive = 0;
    threadPool->tasksQueue = osCreateQueue();
    if (threadPool->tasksQueue == NULL){
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }

    if (!initPthreadStuff(threadPool)) {
        osDestroyQueue(threadPool->tasksQueue);
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    createThreadsArray(threadPool, numOfThreads);
    return threadPool;
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {
    // insert after destroy tp called is not allowed.
    if (!threadPool->shouldWork) return -1;
    task* newTask = (task*)malloc(sizeof(task));
    if (newTask == NULL){
        error();
        return -1;
    }
    newTask->arg = param;
    newTask->function = computeFunc;
    if (pthread_mutex_lock(&threadPool->taskLock) != 0){
        error();
        freeTp(threadPool);
        return -1;
    }
    osEnqueue(threadPool->tasksQueue, newTask);
    // signal that a new task inserted to the queue.
    if (pthread_cond_signal(&threadPool->threadsCond) != 0) {
        error();
        freeTp(threadPool);
        return -1;
    }
    if (pthread_mutex_unlock(&threadPool->taskLock) != 0) {
        error();
        freeTp(threadPool);
        return -1;
    }
    return 0;
}



void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
    if (threadPool == NULL) return;
    threadPool->shouldWaitToFinish = shouldWaitForTasks ? true : false;
    threadPool->shouldWork = !threadPool->shouldWaitToFinish;
    pthread_cond_broadcast(&threadPool->threadsCond);
    int i;
    // join all threads.
    for (i = 0; i < threadPool->numActive; i++){
        pthread_join(threadPool->threads[i],NULL);
    }

    freeTp(threadPool);
}
