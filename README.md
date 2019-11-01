# MapReduce library

This is a simple MapReduce library written in C++, utilizing POSIX threads and synchronization primitives. MapReduce is a distributed computing paradigm for large-scale data processing. It allows applications to run various tasks in parallel, making them scalable and fault-tolerant. This library supports the execution of user-defined `Map` and `Reduce` functions. The user has to include the `mapreduce.h` file in their program and implement the `Map` and `Reduce` functions.

### Design Choices

The execution of this MapReduce library is split into two major phases, as indicated in the name. In the mapping phase the Map function is executed by a set of threads called mappers. These threads are kept in reserve in a thread pool, waiting for work. The threads add key/value pairs to a shared, intermediate data structure that will be used by the reducers later.

#### Threadpool implementation

The thread pool itself is implemented with a struct called `ThreadPool_t`. The data structure contains: 
- a pointer to a task queue of type `ThreadPool_work_queue_t` 
- an array of the thread IDs
- an integer with the number of threads
- a boolean that flags the end of work
- a POSIX mutex lock
- a POSIX condition variable

In the `ThreadPool_create` function, a new `ThreadPool_t` object is created. The mutex lock and the condition variable are initialized. Threads are created using the function `pthread_create` and their IDs are stored in the array of thread IDs. The threads run the `Thread_run` function, in which they will wait for and pick up work.

Available work is stored in the `ThreadPool_work_queue_t` object. This data structure uses a STL priority queue behind the scenes, so that the scheduling algorithm of Longest Job First (LJF) can be implemented. The priority queue is a private member and is thus encapsulated, so that tasks can only be added through the interface of the `ThreadPool_work_queue_t`, which has public functions `get` and `add`. The queue holds pointers to `ThreadPool_work_t` objects, which represent a task or job that a thread will run.

The `ThreadPool_work_t` structure holds a reference to a function, and to the argument for the function. The argument is a filename. These jobs are added to `ThreadPool_work_queue_t` by invoking the `ThreadPool_add_work` function. In this function, the mutex lock is obtained and then the task is added to the queue. Then, the `tasks_available_cond` condition variable is signaled so that threads waiting for more jobs can wake up and pick one up. The lock is then released.

In order to sort the tasks, a comparator struct was written and given to the STL priority queue. The struct compares the size of the files of the `ThreadPool_work_t` objects in the queue, and thus allows the priority queue to sort them according to the LJF algorithm.

In the `Thread_run` function, threads obtain the lock and check the size of the task queue. If it's empty, they are put to sleep waiting on the `tasks_available_cond` condition variable to signal them. This act releases the lock for other threads. Once woken up, the thread receives a job from the threadpool by calling `ThreadPool_get_work`. The thread then releases the lock and then runs the function with the argument specified. Once done, the thread again obtains the lock and checks if the queue is empty, going to sleep if so. This repeats until the threads receive a `NULL` job which signals no more work will be added, and can thus safely terminate.

The `ThreadPool_destroy` function sends the `NULL` job to the queue, and then waits for the threads to terminate. It then performs cleanup, deallocationg memory used.

##### Threadpool changes
A couple of minor changes were made to the original `threadpool.h` file. 
First, the signature of the `Thread_run` function was changed to `void *Thread_run`. This was done to avoid a typecast when providing the newly created threads with a function to run.
Second, `typedef` struct declarations were changed to `struct` declarations. This is because the library was implemented in C++ and `typedef` struct is unnecessary, and lead to some compiler errors. They achieve the same thing, so it was changed.
Lastly, a `LongestJobFirst` struct was added, that provides an operator for comparing file sizes. This was necessary in order to create the priority queue that would sort tasks according to the LJF scheduling policy.

#### Intermediate Data Structure
The intermediate data structures were structs called `Partition`. They were stored in a global array that was dynamically allocated at runtime with the number of reducers as the size. The `Partition` struct contains:
- A STL multimap, with keys and values of type `char *`
- An iterator pointing to the multimap
- A POSIX mutex lock

The STL multimap keeps an ordered list of key/value pairs. The keys do not need to be unique (there can be multiple equivalent keys).
The mapper threads will invoke the user-define `Map` function, which will call `MR_Emit` with the key/value pair it wants to store. The partition the thread should store the pair to is determined by a hash function. Then, the mapper thread obtains the mutex lock to the partition it is assigned to, and the pair is inserted into the multimap. The mutex lock is then released.

#### Time Complexity
The time complexity of the `MR_Emit` function is dependent on the time complexity of inserting a key/value pair into a multimap. According to the STL documentation, multimaps are based on red-black trees. The insertion time for a RBT is O(log n) on average and in the worst case as well. The STL documentation also confirms this time complexity. Therefore the time complexity of `MR_Emit` is O(log n). Additionally, it's useful to note that the time complexity of invoking `MR_Emit` for each key would be O(n log n).

The time complexity of `MR_GetNext` alone is O(1), since it is not doing any iteration itself, it just returns the value that is at the iterator and then increments it. However, it needs to be considered in tandem with `MR_ProcessPartition`, since this function calls the user-defined `Reduce` function which then calls `MR_GetNext`. `MR_Process_Partition` feeds the `Reduce` function the key value at the iterator, and then it calls `MR_GetNext` multiple times to get all the values associated with this key. Thus, this makes the overall time complexity O(n) if n is the number of key/value pairs.

### Synchronization Primitives Used

The synchronization primitives used were the POSIX mutex locks and the POSIX condition variables. 

A mutex lock was used in the threadpool to control access to the task queue, ensuring that only one thread could access it and get a job. They were also used when adding key/value pairs to the shared data structure. There was one for each partition instead of one for all of them, in order to have fine grained locks and to improve efficiency.

A condition variable was used in the threadpool. Threads were set to wait on the `tasks_available_cond` condition variable when there was no work. If work was added, the condition variable would be signaled and a thread would wake up and pick up a new job.

### Testing Strategy

Testing was done in an incremental fashion; each feature was tested during and after its implementation. I would implement one of the functions in the library then I would run the program up to that point to see if it was doing what I expected. I used GDB and print statements to examine the contents of data structures at certain points in the code, to see if everything was behaving correctly. I would print out values in the threads, and within critical sections to see how they were being accessed.

Once there was enough code for the distwc program to work, I mainly used that for my testing. The sample text files given in the assignment description were used and I compared the output of my program with them. After that, I timed the runtime of the code to see if my design was correct and efficient. I then used the easy, medium, hard, and death-incarnate test cases from the forum to test the correctness of my code, ensuring the output matched. Then, I timed the execution as before to ensure efficiency and correctness.

Finally, I used valgrind to check for memory leaks and errors. Valgrind helped me catch memory leaks that I missed when writing the code.

### Sources
http://www.cplusplus.com/reference/map/multimap/insert/
Used to get the time complexity of multimap insertion.

No other external sources, other than class notes, were used.