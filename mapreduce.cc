#include "mapreduce.h"
#include "threadpool.h"
#include <iostream>
#include <unistd.h>
#include <map>

int R;
std::vector<std::multimap<char*, char*>> partitions;
pthread_mutex_t * partition_mutex;

// Main thread that runs this function is master thread
// Map function is user defined
void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    R = num_reducers;
    partition_mutex = new pthread_mutex_t[R];

    for (int i = 0; i < R; i++){
        partitions.push_back(std::multimap<char*, char*>());
        pthread_mutex_init(&partition_mutex[i], 0);
    } 
    
    ThreadPool_t *thread_pool = ThreadPool_create(num_mappers);
    thread_func_t map_pointer = *((thread_func_t) map);

    sleep(2);

    for (int i = 0; i < num_files; i++) {
        ThreadPool_add_work(thread_pool, map_pointer, filenames[i]);
    }

    //Signal work is done
    ThreadPool_add_work(thread_pool, NULL, NULL);

    //Wait for mappers to finish
    for (int i=0; i < num_mappers; ++i) {
        pthread_join(thread_pool->threads[i], NULL);
    }

    std::cout << "Mapper threads finished!" << std::endl;
    // for (int i = 0; i < R; i++){
    //     std::cout << "Partition ";
    //     std::cout << i << std::endl;
    //     for (std::multimap<char*,char*>::iterator it=partitions[i].begin(); it!=partitions[i].end(); ++it) {  
    //         std::cout << (*it).first << " => " << (*it).second << '\n';
    //     }
    // } 

    ThreadPool_destroy(thread_pool);

    //Now make reducer stuff
}

// takes a key and a value associated with it, and writes this pair to a specific partition
// which is determined by passing the key to the MR Partition library function.
void MR_Emit(char *key, char *value) {
    unsigned long partition_number = MR_Partition(key, R);
    
    pthread_mutex_lock(&partition_mutex[partition_number]);
    partitions[partition_number].insert(std::pair<char*,char*>(key,value));
    pthread_mutex_unlock(&partition_mutex[partition_number]);
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