#include "mapreduce.h"
#include "threadpool.h"
#include "partition.h"
#include <pthread.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

int R;
Reducer reduce;
Partition *partitions;

/**
 * @brief Begins the MapReduce process, called by the user-program.
 * 
 * @param num_files - The number of files to process
 * @param filenames - The filenames to process in the mapper threads
 * @param map - The user-defined mapping function, invoked by the mapper threads
 * @param num_mappers - The number of mapper threads desired
 * @param concate - The user-defined reducer function, invoked by the mapper threads
 * @param num_reducers - The number of reducer threads desired
 */
void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    // Save the inputs that are needed in other functions as global variables
    R = num_reducers;
    reduce = concate;

    // Create the partitions
    partitions = new Partition[R];

    // Create the threadpool and give it work
    ThreadPool_t *threadpool = ThreadPool_create(num_mappers);
    thread_func_t map_pointer = *((thread_func_t) map);
    for (int i = 0; i < num_files; i++) {
        ThreadPool_add_work(threadpool, map_pointer, filenames[i]);
    }

    // Waits for work to finish and cleans up the threadpool. This is a blocking call.
    ThreadPool_destroy(threadpool);

    // Create reducer threads
    pthread_t r_threads[R];
    for (int i = 0; i < R; i++){
        pthread_create(&r_threads[i], NULL, (void *(*)(void *)) MR_ProcessPartition, (void *) (intptr_t) i);
    } 

    // Wait for reducers to finish
    for (int i=0; i < R; i++) {
        pthread_join(r_threads[i], NULL);
    }

    // Perform clean-up
    for (int i=0; i < R; i++) {
        pthread_mutex_destroy(&partitions[i].partition_mutex);
    }
    delete []partitions;
}

/**
 * @brief Writes a key/value pair to a partition. Invoked by the user-program
 * that is using the MapReduce library.
 * 
 * @param key - The key
 * @param value - The value associated with the key
 */
void MR_Emit(char *key, char *value) {
    // Copy the char * since the calling function may free it
    char * key_dup = strdup(key);

    // Get the partition for the key from the hash function
    unsigned long partition_number = MR_Partition(key, R);

    // Obtain the lock for the desired partition and add the key/value pair
    pthread_mutex_lock(&partitions[partition_number].partition_mutex);
    partitions[partition_number].map.insert({key_dup, value});
    pthread_mutex_unlock(&partitions[partition_number].partition_mutex);
}

/**
 * @brief A hash function. Given a key and the number of total partitions,
 * returns the partition that the key should be inserted in.
 * 
 * @param key - The key to determine a partition for
 * @param num_partition - The number of available partitions
 */
unsigned long MR_Partition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0') {
        hash = hash * 33 + c;
    }
    return hash % num_partitions;
}

/**
 * @brief Processes the specified partition by runinng the user-defined Reduce function in a loop
 * each time passing it the next unprocessed key. Run by the thread designated to it. Relies on 
 * MR_GetNext incrementing the iterator. Continues until all keys in the partition are processed.
 * 
 * @param partition_number - Index of the partition assigned to the thread that runs it
 */
void MR_ProcessPartition(int partition_number) {
    // Set the iterator to the start of the partition
    partitions[partition_number].iterator = partitions[partition_number].map.begin();
    
    // This will get all the unique keys since MR_GetNext will use the same iterator
    // to iterate through the values for each key
    while (partitions[partition_number].iterator != partitions[partition_number].map.end()) {
        reduce(partitions[partition_number].iterator->first, partition_number);
    }
    pthread_exit(0);
}

/**
 * @brief Gets the next value associated with a key that exists in the partition. In particular, 
 * the ith call to this function should return the ith value associated with the key in the 
 * sorted partition or NULL if i is greater than the number of values associated with the key.
 * Works in tandem with MR_ProcessPartition.
 * 
 * @param key - The key to get the next value for
 * @param partition_number - Index of the partition assigned to the thread that runs it
 */
char *MR_GetNext(char *key, int partition_number) {
    // If iterator points to different key, then we have reached the end of that key's values
    if (partitions[partition_number].iterator == partitions[partition_number].map.end() 
        || strcmp(key, partitions[partition_number].iterator->first) != 0) {
       return NULL;
    }
    char * value = partitions[partition_number].iterator->second;
    partitions[partition_number].iterator++;
    return value;
}