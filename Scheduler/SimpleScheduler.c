#define _POSIX_C_SOURCE 200809L
#include "scheduler.h"
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

typedef struct ProcessInfo {
    pid_t pid;
    struct ProcessInfo* next;

    int startTime;
    int endTime;
    int executed_slices;
    char cmdname[256];

    struct ProcessInfo* next_process;
} ProcessInfo;


static ProcessInfo* processList = NULL;
static ProcessInfo* ready_queue = NULL;
static ProcessInfo* last_in_queue = NULL;

static int NCPU = 1;
static int TSLICE = 200;
static volatile sig_atomic_t tick_count = 0;

#define MAX_RUNNING 512
static pid_t running_pids[MAX_RUNNING];
static int running_count = 0;


static ProcessInfo* findProcess(pid_t pid) {
    ProcessInfo* current = processList;
    while (current) {
        if (current->pid == pid) {
            return current;
        }
        current = current->next_process;
    }
    return NULL;
}


static void enqueue(ProcessInfo* p) {
    if (!p) 
    {
        return;
    }
    
    p->next = NULL;

    if (last_in_queue == NULL) 
    {
        ready_queue = last_in_queue = p;
    } 
    else 
    {
        last_in_queue->next = p;
        last_in_queue = p;
    }
}

static ProcessInfo* dequeue() {
    if (ready_queue == NULL) return NULL;

    ProcessInfo* p = ready_queue;

    ready_queue = ready_queue->next;
    if (ready_queue == NULL) last_in_queue = NULL;


    p->next = NULL;


    return p;
}

void initializeScheduler(int input_NCPU, int input_TSLICE) 
{
    if(input_NCPU > 0)
    {
        NCPU = input_NCPU;
    }
    else
    {
        NCPU = 1;
    }

    if(input_TSLICE > 0)
    {
        TSLICE = input_TSLICE;
    }
    else
    {
        TSLICE  = 200;
    }
    

    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = &roundRobin;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }

    struct itimerval timer;

    long usec = (long)TSLICE * 1000L;

    timer.it_interval.tv_sec = usec / 1000000L;

    timer.it_interval.tv_usec = usec % 1000000L;
    
    timer.it_value = timer.it_interval;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) 
    {
        perror("setitimer error");
        exit(EXIT_FAILURE);
    }
}

void submitProcess(pid_t pid, char *arg) {
    if (pid <= 0)
    {
        return;
    }
        
    ProcessInfo* p = (ProcessInfo*)malloc(sizeof(ProcessInfo));
    if (!p) 
    { 
        perror("malloc"); exit(EXIT_FAILURE); 
    }
    
    p->pid = pid;
    p->next = NULL;
    p->startTime = tick_count;
    p->executed_slices = 0;
    p->endTime = -1;
    p->next_process = NULL;
    
    strncpy(p->cmdname, arg, sizeof(p->cmdname) - 1);
    p->cmdname[sizeof(p->cmdname) - 1] = '\0';


    p->next_process = processList;
    processList = p;


    enqueue(p);
}

void roundRobin(int signum) {
    sigset_t mask, old;

    sigfillset(&mask);
    sigprocmask(SIG_SETMASK, &mask, &old);

    tick_count++;


    for (int i = 0; i < running_count; ) {
        int status;

        pid_t w = waitpid(running_pids[i], &status, WNOHANG);

        if (w > 0) 
        {
            ProcessInfo* p = findProcess(w);
            if (p) 
            {
                p->endTime = tick_count;
            }
            running_pids[i] = running_pids[running_count - 1];

            running_count--;
        } 
        else 
        {
            i++;
        }
    }

    // stoping currently running processes and queuing them
    for (int i = 0; i < running_count; i++) 
    {
        pid_t pid = running_pids[i];

        if (kill(pid, SIGSTOP) == 0) {
            
            ProcessInfo* p = findProcess(pid);
            if (p && p->endTime == -1) 
            { 
                enqueue(p);
            }
        }
    }
    running_count = 0;

    // 3. Schedule new processes from the ready queue
    for (int i = 0; i < NCPU && ready_queue != NULL; i++) {
        ProcessInfo* p = dequeue();
        if (!p) break;

        // check if process not in queue
        if (p->endTime != -1 || kill(p->pid, 0) == -1) {
            if (p->endTime == -1) p->endTime = tick_count;
            i--;
            continue;
        }

        if (p->executed_slices == 0) 
        { 
            kill(p->pid, SIGUSR1);
        } 
        else 
        {
            kill(p->pid, SIGCONT);
        }

        p->executed_slices++;
        running_pids[running_count++] = p->pid;
    }

    sigprocmask(SIG_SETMASK, &old, NULL);
}

void stopScheduler() {

    struct itimerval timer = { {0,0}, {0,0}};
    setitimer(ITIMER_REAL, &timer, NULL);

    //finish all process
    for (int i = 0; i < running_count; i++) {
        kill(running_pids[i], SIGCONT);
    }

    //wait for temination of all process
    ProcessInfo* current = processList;
    while (current) {
        if (current->endTime == -1) 
        {
            
            if (current->executed_slices == 0) kill(current->pid, SIGUSR1);
            kill(current->pid, SIGCONT);

            waitpid(current->pid, NULL, 0);

            current->endTime = tick_count + 1;
        }
        current = current->next_process;
    }
}

void print_stats() 
{
    printf("\n-- Statistics (TSLICE = %dms) --\n", TSLICE);
    printf("%-20s %-8s %-25s %-25s\n", "Name", "PID", "Completion Time", "Wait Time");
    printf("------------------------------------------------\n");

    ProcessInfo* p = processList;
    while (p) {
        int completion_timeslice = p->endTime - p->startTime;

        if (completion_timeslice < 1 && p->executed_slices > 0) {
            completion_timeslice = 1;
        }
        
        int wait_timeslices = completion_timeslice - p->executed_slices;
        if (wait_timeslices < 0) wait_timeslices = 0;


        char completion_str[40];
        char wait_str[40];
        snprintf(completion_str, sizeof(completion_str), "%d x %dms", completion_timeslice, TSLICE);
        snprintf(wait_str, sizeof(wait_str), "%d x %dms", wait_timeslices, TSLICE);


        printf("%-20.20s %-8d %-20s %-20s\n",
            p->cmdname,
            p->pid,
            completion_str,
            wait_str);

        
        ProcessInfo* temp = p;
        p = p->next_process;
        free(temp);
    }
    processList = NULL;
    printf("------------------------------------------------------------\n");
}