#include "threadpool.h"
#include <iostream>

ThreadPool_t *ThreadPool_create(int num) {
    std::priority_queue<ThreadPool_work_t> pq;
    ThreadPool_work_queue_t queue = {pq};
    ThreadPool_t * threadpool = new ThreadPool_t;
    *threadpool = {queue};
    return threadpool;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {
    ThreadPool_work_t job = {func, arg};
    tp->task_queue.add(job);
}

// Get's a task from the task queue and executes it
void *Thread_run(ThreadPool_t *tp) {

}