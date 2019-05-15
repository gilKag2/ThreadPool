#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <sys/types.h>
#include <stdbool.h>
#include "osqueue.h"

typedef struct Task{

    void (*function)(void * arg);
    void* arg;
} task;



typedef struct thread_pool
{
    pthread_t * threads;
    OSQueue* tasksQueue;
    volatile  int  numAlive;
    volatile  int numActive;
    pthread_cond_t  threadsCond;
    pthread_mutex_t  countMutex;
    pthread_mutex_t taskLock;
    bool shouldWork;
    bool shouldWaitToFinish;


}ThreadPool;




ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif