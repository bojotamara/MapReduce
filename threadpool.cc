#include "threadpool.h"
#include <iostream>
#include <unistd.h>


bool largest_job_first::operator() (const ThreadPool_work_t * a, const ThreadPool_work_t * b) {
    struct stat st1;
    struct stat st2;
    // on success 0 is returned -1 for error
    if (a->func == NULL) {
        return true;
    } else if (b->func == NULL) {
        return false;
    }

    stat((char *)a->arg, &st1);
    stat((char *)b->arg, &st2);

    return st1.st_size < st2.st_size;
}

void ThreadPool_work_queue_t::ThreadPool_work_queue_t() {
    std::priority_queue<ThreadPool_work_t *, std::vector<ThreadPool_work_t *>, largest_job_first> pq1;
    pq = pq1;
}

void ThreadPool_work_queue_t::add(ThreadPool_work_t * task) {
    pq.push(task);
}

ThreadPool_work_t * ThreadPool_work_queue_t::get() {
    ThreadPool_work_t * job = pq.top();
    pq.pop();
    return job;
}

// Each thread created by ThreadPool_create runs the Thread run function which gets a task
// from the task queue and executes it (this is done in a loop)
ThreadPool_t *ThreadPool_create(int num) {
    ThreadPool_work_queue_t queue;
    ThreadPool_t * threadpool = new ThreadPool_t;
    *threadpool = {queue, new pthread_t[num], false};

    pthread_cond_init(&threadpool->tasks_available_cond, 0);
    pthread_mutex_init(&threadpool->task_queue_mutex, 0);

    for (int i = 0; i < num; i++) {
        pthread_create(&threadpool->threads[i], NULL, Thread_run, threadpool);
    }

    return threadpool;
}

void ThreadPool_destroy(ThreadPool_t *tp) {
    delete tp->threads;
    pthread_mutex_destroy(&tp->task_queue_mutex);
    pthread_cond_destroy(&tp->tasks_available_cond);
    delete tp;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {
    ThreadPool_work_t * job = new ThreadPool_work_t;
    *job = {func, arg};
    pthread_mutex_lock(&tp->task_queue_mutex);
    tp->task_queue.add(job);
    pthread_cond_signal(&tp->tasks_available_cond);
    pthread_mutex_unlock(&tp->task_queue_mutex);
}

ThreadPool_work_t * ThreadPool_get_work(ThreadPool_t *tp) {
    return tp->task_queue.get();
}

// Get's a task from the task queue and executes it
void *Thread_run(void *tp) {
    ThreadPool_t * threadpool = (ThreadPool_t *) tp;
    
    while (true) {
        pthread_mutex_lock(&threadpool->task_queue_mutex);
        while (threadpool->task_queue.pq.size() == 0 && threadpool->termination_requested == false) {
            pthread_cond_wait(&threadpool->tasks_available_cond, &threadpool->task_queue_mutex);
        }

        ThreadPool_work_t * job = ThreadPool_get_work(threadpool);

        if (job == NULL || job->func == NULL || threadpool->termination_requested) {
            threadpool->termination_requested = true;
            pthread_mutex_unlock(&threadpool->task_queue_mutex);
            pthread_cond_broadcast(&threadpool->tasks_available_cond);
            break;
        }

        pthread_mutex_unlock(&threadpool->task_queue_mutex);

        std::cout << "executing task!" << std::endl;
        job->func(job->arg);
        std::cout << "task finished!" << std::endl;
    }
    pthread_exit(0);
}