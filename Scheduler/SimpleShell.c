#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> 

#define MAX_INPUT_SIZE 1024

volatile sig_atomic_t end_shell = 0;


void sigint_handler(int signum) 
{
    end_shell = 1;
}


void submit_cmd(char **args) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("SimpleShell: fork error");
        return;
    } 
    
    if (pid == 0) 
    { //Child process
        execvp(args[0], args);
        perror("SimpleShell: execvp error"); // execvp only returns on error
        exit(EXIT_FAILURE);
    } 
    else 
    { // Parent process
        
        submitProcess(pid, args[0]);
        printf("SimpleShell: Submitted process %d\n", pid);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s NCPU TSLICE\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int NCPU = atoi(argv[1]);
    int TSLICE = atoi(argv[2]);

    initializeScheduler(NCPU, TSLICE);
    signal(SIGINT, sigint_handler);

    char input[MAX_INPUT_SIZE];
    char *args[128];

    while (!end_shell) {
        printf("SimpleShell~$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) 
        {
            if (feof(stdin)) 
            {
                printf("\n");
                break; 
            }
            continue;
        }

        
        char *token = strtok(input, " \n\t");
        if (!token) continue;

        int arg_count = 0;
        while (token != NULL && arg_count < 127) 
        {
            args[arg_count++] = token;
            token = strtok(NULL, " \n\t");
        }
        args[arg_count] = NULL;

        if (strcmp(args[0], "submit") == 0) 
        {
            if (arg_count > 1) {
                submit_cmd(&args[1]);
            } else {
                printf("Usage: submit <program> [args...]\n");
            }
        } else if (strcmp(args[0], "exit") == 0) 
        {
            break;
        } 
        else 
        {
            
            pid_t pid = fork();
            if (pid == 0) {
                execvp(args[0], args);
                perror("SimpleShell: execvp error");
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                waitpid(pid, NULL, 0);
            } else {
                perror("SimpleShell: fork error");
            }
        }
    }

    printf("\nSimpleShell shutting down. Waiting for all jobs to complete...\n");
    stopScheduler();
    print_stats();

    return 0;
}