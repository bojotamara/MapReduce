#include "partition.h"
#include <string.h>

// Define char * comparison 
bool CStringComparator::operator() (char* a, char* b) {
    return strcmp(a, b) < 0;
}