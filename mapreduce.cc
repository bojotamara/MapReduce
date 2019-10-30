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

void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    // Save the inputs that are needed in other functions as global variables
    R = num_reducers;
    reduce = concate;

    // Create the partitions and init their mutex variables
    // TODO: Convert partition to class and do this initialization maybe?
    partitions = new Partition[R];
    for (int i = 0; i < R; i++) {
        pthread_mutex_init(&partitions[i].partition_mutex, 0);
    }

    // Create the threadpool and give it work
    ThreadPool_t *threadpool = ThreadPool_create(num_mappers);
    thread_func_t map_pointer = *((thread_func_t) map);
    for (int i = 0; i < num_files; i++) {
        ThreadPool_add_work(threadpool, map_pointer, filenames[i]);
    }

    // Waits for work to finish and cleans up the threadpool. This is a blocking call.
    ThreadPool_destroy(threadpool);

    pthread_t r_threads[R];
    for (int i = 0; i < R; i++){
        partitions[i].iterator = partitions[i].map.begin();
        pthread_create(&r_threads[i], NULL, (void *(*)(void *)) MR_ProcessPartition, (void *) (intptr_t)i);
    } 

    // Wait for reducers to finish
    for (int i=0; i < R; i++) {
        pthread_join(r_threads[i], NULL);
    }

    delete []partitions;
}

// takes a key and a value associated with it, and writes this pair to a specific partition
// which is determined by passing the key to the MR Partition library function.
void MR_Emit(char *key, char *value) {
    //This is to ensure null characters don't get thru. Idk why this started to happen
    if (key != NULL && key[0] == '\0') {
        return;
    }

    // Copy the char * since the calling function may free it
    char * key_dup = strdup(key);

    // Get the partition for the key from the hash function
    unsigned long partition_number = MR_Partition(key, R);

    // Obtain the lock for the desired partition and add the key/value pair
    pthread_mutex_lock(&partitions[partition_number].partition_mutex);
    partitions[partition_number].map.insert({key_dup, value});
    pthread_mutex_unlock(&partitions[partition_number].partition_mutex);
}

unsigned long MR_Partition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0') {
        hash = hash * 33 + c;
    }
    return hash % num_partitions;
}

// Takes the index of the partition assigned to the thread that runs
// it. It invokes the user-defined Reduce function in a loop, each time passing it the next unprocessed key. This
// continues until all keys in the partition are processed.
void MR_ProcessPartition(int partition_number) {
    while (partitions[partition_number].iterator != partitions[partition_number].map.end()) {
        reduce(partitions[partition_number].iterator->first, partition_number);
    }
    pthread_exit(0);
}

// Takes a key and a partition number, and returns a value associated with the key
// that exists in that partition. In particular, the ith call to this function should return the 
// ith value associated with the key in the sorted partition or NULL if i is greater than the number
// of values associated with the key.
char *MR_GetNext(char *key, int partition_number) {
    if (partitions[partition_number].iterator == partitions[partition_number].map.end() 
        || strcmp(key, partitions[partition_number].iterator->first) != 0) {
       return NULL;
    }
    char * value = partitions[partition_number].iterator->second;
    partitions[partition_number].iterator++;
    return value;
}