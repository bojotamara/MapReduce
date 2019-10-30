#pragma once

#include <pthread.h>
#include <map>
#include <set>
#include <string.h>

struct cstrless {
    bool operator()(char* a, char* b) {
      return strcmp(a, b) < 0;
    }
};

struct Partition
{   
    std::multimap<char *, char *, cstrless> map;
    std::set<char *, cstrless> keys;
    std::multimap<char *, char *, cstrless>::iterator iterator;
    pthread_mutex_t partition_mutex;
};