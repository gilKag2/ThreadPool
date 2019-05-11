#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <sys/types.h>
#include <stdbool.h>
#include "osqueue.h"

typedef struct Task{
    struct task * prevTask;
    void (*function)(void * arg);
    void* arg;
} task;



typedef struct thread_pool
{
    struct thread ** threads;
    OSQueue* tasksQueue;
    volatile  int  numAlive;
    volatile  int numActive;
    pthread_cond_t  threadAreIdle;
    pthread_mutex_t  countMutex;
    pthread_mutex_t taskLock;
    bool shouldWork;
    bool shouldWaitToFinish;


}ThreadPool;

typedef struct Thread{
    ThreadPool* threadPool;
    pthread_t pThread;

} thread;



ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif