#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <pthread.h>
#include <queue>
#include <vector>

//Defines a function that a thread can run
typedef void (*thread_func_t)(void *arg);

// Defines the work to do
struct ThreadPool_work_t {
    thread_func_t func;              // The function pointer
    void *arg;                       // The arguments for the function
};

// Comparator struct that allows the sorting of work based on the LJF algorithm
struct LongestJobFirst {
    bool operator() (const ThreadPool_work_t * a, const ThreadPool_work_t * b);
};

// The work queue that the threadpool receives tasks from
struct ThreadPool_work_queue_t {
    private:
        // the underlying implementation is a priority queue
        std::priority_queue<ThreadPool_work_t *, std::vector<ThreadPool_work_t *>, LongestJobFirst> queue;
        int size;
    public:
        ThreadPool_work_queue_t(): queue(), size(0) {};
        void add(ThreadPool_work_t * task);
        ThreadPool_work_t * get();
        int getSize();
};

// The threadpool struct
struct ThreadPool_t {
    ThreadPool_work_queue_t * task_queue;
    pthread_t * threads;
    int num_threads;
    bool termination_requested;
    pthread_mutex_t task_queue_mutex;
    pthread_cond_t tasks_available_cond;
};


/**
* A C style constructor for creating a new ThreadPool object
* Parameters:
*     num - The number of threads to create
* Return:
*     ThreadPool_t* - The pointer to the newly created ThreadPool object
*/
ThreadPool_t *ThreadPool_create(int num);

/**
* A C style destructor to destroy a ThreadPool object
* Parameters:
*     tp - The pointer to the ThreadPool object to be destroyed
*/
void ThreadPool_destroy(ThreadPool_t *tp);

/**
* Add a task to the ThreadPool's task queue
* Parameters:
*     tp   - The ThreadPool object to add the task to
*     func - The function pointer that will be called in the thread
*     arg  - The arguments for the function
* Return:
*     true  - If successful
*     false - Otherwise
*/
bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg);

/**
* Get a task from the given ThreadPool object
* Parameters:
*     tp - The ThreadPool object being passed
* Return:
*     ThreadPool_work_t* - The next task to run
*/
ThreadPool_work_t * ThreadPool_get_work(ThreadPool_t *tp);

/**
* Run the next task from the task queue
* Parameters:
*     tp - The ThreadPool Object this thread belongs to
*/
void *Thread_run(void *tp);
#endif
