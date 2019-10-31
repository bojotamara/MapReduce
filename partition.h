#pragma once

#include <pthread.h>
#include <map>

// Comparator struct that allows the sorting of char * keys
struct CStringComparator {
    bool operator()(char* a, char* b);
};

// Struct for the intermediate data structure used by the mappers and reducer
struct Partition
{   
    // holds the key/value pairs
    std::multimap<char *, char *, CStringComparator> map;
    // pointer that references the current key/value pair the reducer is processing
    std::multimap<char *, char *, CStringComparator>::iterator iterator;
    // mutex lock for the partition
    pthread_mutex_t partition_mutex = PTHREAD_MUTEX_INITIALIZER;
};