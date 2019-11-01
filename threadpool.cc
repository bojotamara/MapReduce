#include "threadpool.h"
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

// Compares the jobs based on file size. Larger file size means a larger job.
bool LargestJobFirst::operator() (const ThreadPool_work_t * a, const ThreadPool_work_t * b) {
    struct stat st1, st2;

    // The 'null' job that signals the end of work always goes last
    if (a->func == NULL) {
        return true;
    } else if (b->func == NULL) {
        return false;
    }

    stat((char *)a->arg, &st1);
    stat((char *)b->arg, &st2);

    return st1.st_size < st2.st_size;
}

/**
 * @brief Constructor for the threadpool work queue.
 */
void ThreadPool_work_queue_t::ThreadPool_work_queue_t() {
    std::priority_queue<ThreadPool_work_t *, std::vector<ThreadPool_work_t *>, LargestJobFirst> pq;
    queue = pq;
    size=0;
}

/**
 * @brief Adds a task to the work queue.
 * 
 * @param task - Pointer to the ThreadPool_work_t task
 */
void ThreadPool_work_queue_t::add(ThreadPool_work_t * task) {
    queue.push(task);
    size++;
}

/**
 * @brief Gets the next task from the work queue and pops it off the queue.
 */
ThreadPool_work_t * ThreadPool_work_queue_t::get() {
    if (size == 0) {
        return NULL;
    }
    ThreadPool_work_t * job = queue.top();
    queue.pop();
    size--;
    return job;
}

/**
 * @brief Returns the size of the work queue
 */
int ThreadPool_work_queue_t::getSize() {
    return size;
}

/**
 * @brief Creates and returns a threadpool. The threads begin running and get tasks in Thread_run.
 * 
 * @param num - The number of threads to create
 */
ThreadPool_t *ThreadPool_create(int num) {
    ThreadPool_work_queue_t * queue = new ThreadPool_work_queue_t;
    ThreadPool_t * threadpool = new ThreadPool_t;
    *threadpool = {queue, new pthread_t[num], num, false};

    pthread_cond_init(&threadpool->tasks_available_cond, 0);
    pthread_mutex_init(&threadpool->task_queue_mutex, 0);

    for (int i = 0; i < num; i++) {
        pthread_create(&threadpool->threads[i], NULL, Thread_run, threadpool);
    }

    return threadpool;
}

/**
 * @brief Waits for the tasks in the given threadpool to complete and then destroys
 * and cleans up the threadpool.
 * 
 * @param tp - The thread pool object to destroy
 */
void ThreadPool_destroy(ThreadPool_t *tp) {
    //Signal work is done with a 'null' job
    ThreadPool_add_work(tp, NULL, NULL);

    for (int i=0; i < tp->num_threads; i++) {
        pthread_join(tp->threads[i], NULL);
    }

    delete tp->threads;
    delete tp->task_queue;
    pthread_mutex_destroy(&tp->task_queue_mutex);
    pthread_cond_destroy(&tp->tasks_available_cond);
    delete tp;
}

/**
 * @brief Adds work to the given threadpool
 * 
 * @param tp - Pointer to the threadpool
 * @param func - Function user wants thread to perform
 * @param arg - Argument to the function
 */
bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {
    ThreadPool_work_t * job = new ThreadPool_work_t;
    *job = {func, arg};
    pthread_mutex_lock(&tp->task_queue_mutex);
    tp->task_queue->add(job);
    pthread_cond_signal(&tp->tasks_available_cond);
    pthread_mutex_unlock(&tp->task_queue_mutex);
    // TODO: ????
    return true;
}

/**
 * @brief Returns the next task from the threadpool
 * 
 * @param task - Pointer to the threadpool
 */
ThreadPool_work_t * ThreadPool_get_work(ThreadPool_t *tp) {
    return tp->task_queue->get();
}

/**
 * @brief Runs the next task in the given threadpool. Threads run this function
 * 
 * @param tp - Pointer to the threadpool to get tasks from
 */
void *Thread_run(void *tp) {
    ThreadPool_t * threadpool = (ThreadPool_t *) tp;

    while (true) {
        pthread_mutex_lock(&threadpool->task_queue_mutex);

        while (threadpool->task_queue->getSize() == 0 && threadpool->termination_requested == false) {
            pthread_cond_wait(&threadpool->tasks_available_cond, &threadpool->task_queue_mutex);
        }

        ThreadPool_work_t * job = ThreadPool_get_work(threadpool);

        // If the 'null' job was received we must signal to the rest of the threads to exit when they can
        if (job == NULL || job->func == NULL || threadpool->termination_requested) {
            threadpool->termination_requested = true;
            pthread_mutex_unlock(&threadpool->task_queue_mutex);
            pthread_cond_broadcast(&threadpool->tasks_available_cond);
            break;
        }

        pthread_mutex_unlock(&threadpool->task_queue_mutex);

        job->func(job->arg);
        delete job;
    }

    pthread_exit(0);
}