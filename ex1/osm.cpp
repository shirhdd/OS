//
// Created by itayyamin on 3/20/23.
//
#include <sys/time.h>
#include <ctime>
#include "osm.h"

/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_operation_time(unsigned int iterations){
    if (iterations == 0){
        return -1;
    }
    struct timeval start, end;
    int first_time = gettimeofday(&start, NULL);
    if (first_time == -1){
        return -1;
    }

    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    int e = 0;
    int f = 0;

    unsigned int rounded_iterations = (iterations % 6 == 0) ?
            iterations : iterations + 6 - iterations % 6;
    for (unsigned int i = 0; i < rounded_iterations; ++i) {
        a++;
        b++;
        c++;
        d++;
        e++;
        f++;
    }

    int second_time = gettimeofday(&end, NULL);
    if (second_time == -1){
        return -1;
    }

// Calculate the time elapsed in microseconds
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    long elapsed = (seconds * 1000000) + microseconds;
    return (double)(elapsed * 1000) / (rounded_iterations * 6);
}
void empty(){}
/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_function_time(unsigned int iterations){
    if (iterations == 0){
        return -1;
    }

    struct timeval start, end;


    int first_time = gettimeofday(&start, NULL);
    if (first_time == -1){
        return -1;
    }
    unsigned int rounded_iterations = (iterations % 6 == 0) ?
                                      iterations : iterations + 6 - iterations % 6;
    for (unsigned int i = 0; i < rounded_iterations; ++i) {
        empty();
        empty();
        empty();
        empty();
        empty();
        empty();
    }

    int second_time = gettimeofday(&end, NULL);
    if (second_time == -1){
        return -1;
    }
// Calculate the time elapsed in microseconds
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    long elapsed = (seconds * 1000000) + microseconds;
    return (double)(elapsed * 1000) / (rounded_iterations * 6);
}

/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_syscall_time(unsigned int iterations){
    if (iterations == 0){
        return -1;
    }
    struct timeval start, end;

    int first_time = gettimeofday(&start, NULL);
    if (first_time == -1){
        return -1;
    }
    unsigned int rounded_iterations = (iterations % 6 == 0) ?
                                      iterations : iterations + 6 - iterations % 6;
    for (unsigned int i = 0; i < rounded_iterations; ++i) {
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
    }

    int second_time = gettimeofday(&end, NULL);
    if (second_time == -1){
        return -1;
    }

// Calculate the time elapsed in microseconds
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    long elapsed = (seconds * 1000000) + microseconds;
    return (double)(elapsed * 1000) / (rounded_iterations * 6);
}