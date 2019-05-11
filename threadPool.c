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
    osDestroyQueue(tp->tasksQueue);
    pthread_mutex_destroy(&tp->taskLock);
    pthread_mutex_destroy(&tp->countMutex);
    pthread_cond_destroy(&tp->threadAreIdle);
    free(tp->threads);
    free(tp);
}

task* getNextTask(ThreadPool* tp){
    if (pthread_mutex_lock(&tp->taskLock) != 0) {
        error();
        return NULL;
    }
    task*  nextTask = osDequeue(tp->tasksQueue);
    if (pthread_mutex_unlock(&tp->taskLock) != 0) {
        error();
        return  NULL;
    }
    return  nextTask;
}

// assigning a thread to execute a task.
void* assignThread(thread * th) {
    ThreadPool* threadPool = (ThreadPool *) th->threadPool;
    if(pthread_mutex_lock(&threadPool->countMutex) != 0) {
        error();
        return NULL;
    }
    // increase count;
    threadPool->numAlive++;
    if (pthread_mutex_unlock(&threadPool->countMutex) != 0) {
        error();
        return  NULL;
    }
    
    while (threadPool->shouldWork){
        if (osIsQueueEmpty(threadPool->tasksQueue)){
            //
            if (threadPool->shouldWaitToFinish) // end here somehow without returning NULL
            while (threadPool->shouldWork &&  )
            
        }
        if (pthread_mutex_lock(&threadPool->countMutex) != 0) {
            error();
            return NULL;
        }
        // one more active thread.
        threadPool->numActive++;
        if (pthread_mutex_unlock(&threadPool->countMutex) != 0){
            error();
            return NULL;
        }
        // get the next task and execute.
        task * nextTask = getNextTask(threadPool);
        if (nextTask == NULL)
            return NULL;
        nextTask->function(nextTask->arg);
        free(nextTask);
    }
    if (pthread_mutex_lock(&threadPool->countMutex) != 0){
        error();
        return NULL;
    }
    threadPool->numActive--;
    // all thread are idle(maybe some are block), so we signal to realease from the block.
    if (threadPool->numActive == 0) {
        if (pthread_cond_signal(&threadPool->threadAreIdle) != 0){
            error();
            return  NULL;
        }
    }
   if (pthread_mutex_unlock(&threadPool->countMutex) != 0){
       error();
       return  NULL;
   }
}

// initialize the thread.
void* initThread(thread** th, ThreadPool* thPool) {
    *th = (thread*)malloc(sizeof(thread));
    if (*th == NULL){
        error();
        return NULL;
    }
   
    (*th)->threadPool = thPool;
    if (pthread_create(&(*th)->pThread, NULL, (void*)assignThread, (*th)) != 0) {
        error();
        return NULL;
    }
}

void* initPthreadStuff(ThreadPool* th) {
    if (pthread_mutex_init(&th->countMutex, NULL) != 0) {
        error();
        return NULL;
    }
    if (pthread_mutex_init(&th->taskLock, NULL) != 0){
        error();
        if (pthread_mutex_destroy(&th->countMutex) != 0){
            error();
        }
        return NULL;
    }
        
    if (pthread_cond_init(&th->threadAreIdle, NULL) != 0){
        error();
        if (pthread_mutex_destroy(&th->taskLock) != 0) 
            error();
        if (pthread_mutex_destroy(&th->countMutex) != 0)
            error();
        return NULL;
    }
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
    threadPool->shouldWaitToFinish = false;
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
   /* // init mutex and cond.
    if (pthread_mutex_init(&threadPool->countMutex, NULL) != 0) 
    }
    pthread_mutex_init(&threadPool->taskLock, NULL);
    pthread_cond_init(&threadPool->threadAreIdle, NULL);
*/
   if (initPthreadStuff(threadPool) == NULL){
       osDestroyQueue(threadPool->tasksQueue);
       free(threadPool);
       return NULL;
   }
    int i;
    for (i = 0; i < numOfThreads; i++) {
        if (initThread((thread **) &threadPool->threads[i], threadPool) == NULL) {
            for(i = 0; i <= threadPool->numAlive; i++) {                       ///////////////// not sure here!!!1
                if (threadPool->threads[i] != NULL) {
                    free(threadPool->threads[i]);
                }
            }
            freeTp(threadPool);
            return NULL;
        }
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
    if (threadPool == NULL) return;
    threadPool->shouldWaitToFinish = shouldWaitForTasks ? true : false;
    threadPool->shouldWork = !threadPool->shouldWaitToFinish;
    
}
