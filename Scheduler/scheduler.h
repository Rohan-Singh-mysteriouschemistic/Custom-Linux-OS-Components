#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <sys/types.h>


void initializeScheduler(int NCPU, int TSLICE);
void submitProcess(pid_t pid,char *arg);
void roundRobin(int signum);
void stopScheduler();
void print_stats();

#endif // SCHEDULER_H
