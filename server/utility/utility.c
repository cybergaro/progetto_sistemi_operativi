#include "utility.h"
#include <pthread.h>


int new_book_number() {
    return (int)time(NULL);
}