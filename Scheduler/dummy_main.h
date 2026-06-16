#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int dummy_main(int argc, char **argv);

void signal_handler(int signum) {
    // signal handler function
    if (signum == SIGUSR1) {
        printf("Received SIGUSR1 signal. Resuming execution.\n");
        //signal recieved so we resume
    }
}

// HAVE LEFT THE PRINT STATEMENTS USED WHILE DEBUGGING FOR BETTER CLARITY.

int main(int argc, char **argv) {
        signal(SIGUSR1,signal_handler);
        pause(); //pausing here to wait for a signal
        int ret = dummy_main(argc, argv);
        return ret;
}
#define main dummy_main