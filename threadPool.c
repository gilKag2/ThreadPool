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

void* assignThread(thread * th) {
    ThreadPool* threadPool = (ThreadPool *) th->threadPool;
    while (threadPool->shouldWork){
        if (osIsQueueEmpty(threadPool->tasksQueue))
    }
}

int initThread(thread** th, int id, struct ThreadPool* thPool) {
    *th = (thread*)malloc(sizeof(thread));
    if (*th == NULL){
        error();
        return -1;
    }
    (*th)->id = id;
    (*th)->threadPool = thPool;
    pthread_create(&(*th)->pThread, NULL, (void*)assignThread, (*th));
}

ThreadPool* tpCreate(int numOfThreads) {
    ThreadPool* threadPool;
    // allocate mem to the th_pool.
    threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
    // error in allocation.
    if (threadPool == NULL) {
        error();
        free(threadPool);
        return NULL;
    }
    threadPool->shouldWork = true;
    threadPool->numActive = 0;
    threadPool->numAlive = 0;
    threadPool->tasksQueue = osCreateQueue();
    // allocate n thread in the th_pool.
    threadPool->threads = (thread**)malloc(numOfThreads * sizeof(thread*));
    // error in allocation.
    if (threadPool->threads == NULL) {
        error();
        osDestroyQueue(threadPool->tasksQueue);
        free(threadPool);
        return NULL;
    }
    // init mutex and cond.
    pthread_mutex_init(&threadPool->thpLock, NULL);
    pthread_cond_init(&threadPool->threadAreIdle, NULL);

    int i;
    for (i = 0; i < numOfThreads; i++) {
        initThread(&threadPool->threads[i], i);
    }

    // wait for all the threads to init.
    while (1) {
        if (threadPool->numAlive == numOfThreads) {
            return threadPool;
        }
    }
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {

}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {

}