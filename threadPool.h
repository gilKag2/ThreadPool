
/*
 * Gil Kagan
 * 315233221
 */

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <sys/param.h>
#include <stdbool.h>
#include "osqueue.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
typedef struct task
{
    void (*computeFunc)(void*);
    void *parameters;
}Task;

typedef struct ThreadPool
{
    OSQueue* tasksQueue;
    OSQueue* pThreadsQueue;
    pthread_mutex_t tasksLock;
    pthread_mutex_t newTaskLock;
    pthread_mutex_t pThreadLock;
    pthread_cond_t taskCond;
    volatile bool finish;
    volatile bool shouldWait;

}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif


