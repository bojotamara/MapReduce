#include "threadpool.h"
#include <iostream>

bool largest_job_first::operator() (ThreadPool_work_t const &a, ThreadPool_work_t const &b) {
    struct stat st1;
    struct stat st2;
    // on success 0 is returned -1 for error
    stat((char *)a.arg, &st1);
    stat((char *)b.arg, &st2);

    return st1.st_size < st2.st_size;
}

void ThreadPool_work_queue_t::add(ThreadPool_work_t task) {
    pq.push(task);
}

ThreadPool_t *ThreadPool_create(int num) {
    std::priority_queue<ThreadPool_work_t, std::vector<ThreadPool_work_t>, largest_job_first> pq;
    ThreadPool_work_queue_t queue = {pq};
    ThreadPool_t * threadpool = new ThreadPool_t;
    *threadpool = {queue};
    return threadpool;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {
    char * text = (char *)arg;
    ThreadPool_work_t job = {func, arg};
    tp->task_queue.add(job);
}

// Get's a task from the task queue and executes it
void *Thread_run(ThreadPool_t *tp) {

}