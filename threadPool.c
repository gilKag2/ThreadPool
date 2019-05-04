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

task* getNextTask(ThreadPool* tp){
    pthread_mutex_lock(&tp->taskLock);
    task*  nextTask = osDequeue(tp->tasksQueue);
    pthread_mutex_unlock(&tp->taskLock);
    return  nextTask;
}

// assigning a thread to execute a task.
void* assignThread(thread * th) {
    ThreadPool* threadPool = (ThreadPool *) th->threadPool;
    while (threadPool->shouldWork){
        if (osIsQueueEmpty(threadPool->tasksQueue)){
            // do something here about destroy
        }
        pthread_mutex_lock(&threadPool->countActiveMutex);
        // one more active thread.
        threadPool->numActive++;
        pthread_mutex_unlock(&threadPool->countActiveMutex);
        // get the next task and execute.
        task * nextTask = getNextTask(threadPool);
        nextTask->function(nextTask->arg);
        free(nextTask);
    }
    pthread_mutex_lock(&threadPool->countActiveMutex);
    threadPool->numActive--;
    // all thread are idle(maybe some are block), so we signal to realease from the block.
    if (threadPool->numActive == 0) {
        pthread_cond_signal(&threadPool->threadAreIdle);
    }
    pthread_mutex_unlock(&threadPool->countActiveMutex);
}

// initialize the thread.
int initThread(thread** th, int id, ThreadPool* thPool) {
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
    if (numOfThreads < 1) {
        exit(0);
    }
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
    threadPool->threads = (struct thread **)malloc(numOfThreads * sizeof(struct thread *));
    // error in allocation.
    if (threadPool->threads == NULL) {
        error();
        osDestroyQueue(threadPool->tasksQueue);
        free(threadPool);
        return NULL;
    }
    // init mutex and cond.
    pthread_mutex_init(&threadPool->countActiveMutex, NULL);
    pthread_mutex_init(&threadPool->taskLock, NULL);
    pthread_cond_init(&threadPool->threadAreIdle, NULL);

    int i;
    for (i = 0; i < numOfThreads; i++) {
        initThread((thread **) &threadPool->threads[i], i, threadPool);
    }

    // wait for all the threads to init.
    while (1) {
        if (threadPool->numAlive == numOfThreads) {
            return threadPool;
        }
    }
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {

    task* newTask = (task*)malloc(sizeof(task));
    if (newTask == NULL){
        error();
        exit(0);
    }
    newTask->arg = param;
    newTask->function = computeFunc;
    pthread_mutex_lock(&threadPool->taskLock);
    osEnqueue(threadPool->tasksQueue, newTask);
    pthread_mutex_unlock(&threadPool->taskLock);
}


void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {

}
