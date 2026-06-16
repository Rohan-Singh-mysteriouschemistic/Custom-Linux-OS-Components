#include <stdio.h>
#include <unistd.h>      // Required for getpid()
#include "dummy_main.h"  // CRUCIAL: This makes it work with the scheduler

// This function is automatically renamed by the header file
int main(int argc, char **argv) {
    printf("Job with PID %d starting...\n", getpid());
    fflush(stdout); // Ensure the message prints immediately

    // A simple busy-loop to use CPU time for a while
    volatile unsigned long long i;
    for (i = 0; i < 2000000000ULL; i++);

    printf("Job with PID %d finished.\n", getpid());
    fflush(stdout);

    return 0;
}