#include "mapreduce.h"
#include "threadpool.h"
#include <iostream>


// Main thread that runs this function is master thread
// Map function is user defined
void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    ThreadPool_t *thread_pool = ThreadPool_create(num_mappers);
    thread_func_t map_pointer = *((thread_func_t) map);

    for (int i = 0; i < num_files; i++) {
        ThreadPool_add_work(thread_pool, map_pointer, filenames[i]);
    }
}

// takes a key and a value associated with it, and writes this pair to a specific partition
// which is determined by passing the key to the MR Partition library function.
void MR_Emit(char *key, char *value) {
    unsigned long partition_number = MR_Partition(key, 0);
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

}

// Takes a key and a partition number, and returns a value associated with the key
// that exists in that partition. In particular, the ith call to this function should return the 
// ith value associated with the key in the sorted partition or NULL if i is greater than the number
// of values associated with the key.
char *MR_GetNext(char *key, int partition_number) {

}