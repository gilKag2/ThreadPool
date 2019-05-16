/*
 * Gil Kagan
 * 315233221
 */
#include "threadPool.h"
#include <stdio.h>


#define SUCCESS 0
#define ERROR "Error in system call\n"
#define ERROR_SIZE strlen(ERROR)

void error() {
    write(fileno(stderr), ERROR, ERROR_SIZE);
}
void freeTp(ThreadPool *tp) {
    if (pthread_mutex_lock(&tp->pThreadLock) != 0) {
        error();
        exit(EXIT_FAILURE);
    }
    while (!osIsQueueEmpty(tp->pThreadsQueue)) {
        if (pthread_cond_broadcast(&tp->taskCond) != SUCCESS) {
            error();
        }
        pthread_t *pt= (pthread_t *) osDequeue(tp->pThreadsQueue);
        if (pt != NULL){
            // join thread
            pthread_join(*pt, NULL);
            free(pt);
        }
       
    }
    if (pthread_mutex_unlock(&tp->pThreadLock) != 0){
        error();
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_destroy(&tp->pThreadLock) != 0) {
        error();
        exit(EXIT_FAILURE);
    }
    osDestroyQueue(tp->pThreadsQueue);
    if (pthread_mutex_lock(&tp->tasksLock) != 0) {
        error();
        exit(EXIT_FAILURE);
    }
    while (!osIsQueueEmpty(tp->tasksQueue)) {
         Task* task = osDequeue(tp->tasksQueue);
         if (task != NULL) {
            free(task);
        }
    }
    if (pthread_mutex_unlock(&tp->tasksLock) != 0){
        error();
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_destroy(&tp->tasksLock) != 0) {
        error();
        exit(EXIT_FAILURE);
    }
    osDestroyQueue(tp->tasksQueue);

   
    if (pthread_cond_destroy(&tp->taskCond) != 0) {
        error();
        exit(EXIT_FAILURE);
    }
    free(tp);
}


int initPthreadStuff(ThreadPool* tp) {
    if (pthread_mutex_init(&tp->pThreadLock, NULL) != SUCCESS) {
        error();
        return 0;
    }
    if (pthread_mutex_init(&tp->tasksLock, NULL) != SUCCESS){
        error();
        if (pthread_mutex_destroy(&tp->pThreadLock) != SUCCESS){
            error();
        }
        return 0;
    }
    if (pthread_mutex_init(&tp->newTaskLock, NULL) != SUCCESS){
        error();
        if (pthread_mutex_destroy(&tp->pThreadLock) != SUCCESS){
            error();
        }
        if (pthread_mutex_destroy(&tp->tasksLock) != SUCCESS)
            error();
    }

    if (pthread_cond_init(&tp->taskCond, NULL) != SUCCESS){
        error();
        if (pthread_mutex_destroy(&tp->tasksLock) != SUCCESS)
            error();
        if (pthread_mutex_destroy(&tp->pThreadLock) != SUCCESS)
            error();
        if (pthread_mutex_destroy(&tp->newTaskLock) != SUCCESS) {
            error();
        }
        return 0;
    }
    return 1;
}


//wait until new Task enters the queue.
void waitForNewTask(ThreadPool *threadPool) {
    if (pthread_mutex_lock(&threadPool->newTaskLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    if (!threadPool->finish) {
        if (pthread_cond_wait(&threadPool->taskCond, &threadPool->newTaskLock) != SUCCESS) {
            error();
            freeTp(threadPool);
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_mutex_unlock(&threadPool->newTaskLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
}

// checks if the tasks queue is empty.
bool isTaskQueueEmpty(ThreadPool *threadPool) {
    bool isEmpty = NULL;
    if (pthread_mutex_lock(&threadPool->tasksLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    isEmpty = osIsQueueEmpty(threadPool->tasksQueue);
    if (pthread_mutex_unlock(&threadPool->tasksLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    return isEmpty;
}
// returns the next task from the tasks queue.
Task *getNextTask(ThreadPool *threadPool) {
    //lock the mutex
    if (pthread_mutex_lock(&threadPool->tasksLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    Task *Task = osDequeue(threadPool->tasksQueue);
    if (pthread_mutex_unlock(&threadPool->tasksLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    return Task;
}

//Run Task if exist and free its memory
void runTask(Task *task) {
    if (task != NULL) {
        task->computeFunc(task->parameters);
        free(task);
    }
}

// waits for tasks, and runs them on the threads until destruction is called.
void *startWorking(void *threadPool) {
    ThreadPool *tp = (ThreadPool *) threadPool;
    while (tp->finish == false) {
        //check if queue is empty
        if (isTaskQueueEmpty(tp)) {
            waitForNewTask(tp);
        }
       runTask(getNextTask(tp));
    }
    if (tp->shouldWait) {
        while (!isTaskQueueEmpty(tp)) {
            runTask(getNextTask(tp));
        }
    }
}
// create the thread pool
ThreadPool *tpCreate(int numOfThreads) {
    ThreadPool *threadPool = malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        error();
        _exit(EXIT_FAILURE);
    }

    if (!initPthreadStuff(threadPool)) {
        free(threadPool);
        exit(EXIT_FAILURE);
    }
    threadPool->tasksQueue = osCreateQueue();
    if (threadPool->tasksQueue == NULL) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    threadPool->pThreadsQueue = osCreateQueue();
    if (threadPool->pThreadsQueue == NULL) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&threadPool->taskCond, NULL) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    threadPool->finish = false;
    threadPool->shouldWait = false;

    ///// threads init from here.

    if (pthread_mutex_lock(&threadPool->pThreadLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    //create each thread
    int i;
    for (i = 0; i < numOfThreads; i++) {
        pthread_t *pt = (pthread_t *)malloc(sizeof(pthread_t));
        if (pt == NULL) {
            error();
            _exit(EXIT_FAILURE);
        }
        osEnqueue(threadPool->pThreadsQueue, pt);

        if (pthread_create(pt, NULL, startWorking, threadPool) != SUCCESS) {
            error();
            freeTp(threadPool);
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_mutex_unlock(&threadPool->pThreadLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    return threadPool;
}



//Send signal when new Task entered the queue
void sendSignal(ThreadPool *threadPool) {

    if (pthread_mutex_lock(&threadPool->newTaskLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_signal(&threadPool->taskCond) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_unlock(&threadPool->newTaskLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
}

// addss the task to the tasks queue.
void addTaskToQueue(ThreadPool *threadPool, Task *task) {
   
    if (pthread_mutex_lock(&threadPool->tasksLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    // add to queue.
    osEnqueue(threadPool->tasksQueue, task);
    if (pthread_mutex_unlock(&threadPool->tasksLock) != SUCCESS) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    // let others know that a new task arrived.
    sendSignal(threadPool);
}

//insert new task
int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    // cant add another task if the tp is destroyed.
    if (threadPool->finish) {
        return -1;
    }
    Task *Task = malloc(sizeof(Task));
    // allocaation failed.
    if (Task == NULL) {
        error();
        freeTp(threadPool);
        exit(EXIT_FAILURE);
    }
    Task->computeFunc = computeFunc;
    Task->parameters = param;
    addTaskToQueue(threadPool, Task);
    return 0;
}


//destroy the thread pool
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    if (threadPool->finish) {
        return;
    }
    // destroy the threadPool.
    threadPool->finish = true;
 
    threadPool->shouldWait = shouldWaitForTasks ? true : false;
    freeTp(threadPool);
}

