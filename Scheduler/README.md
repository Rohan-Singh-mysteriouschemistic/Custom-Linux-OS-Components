# Simple Scheduler Implementation

**Course:** Operating Systems  
**Team Members:**  
- Rohan Singh (2024477)  
- Agrim Verma (2024047)

---

## Overview

This project implements a **Simple Scheduler** program that demonstrates **Round-Robin CPU scheduling** for user-submitted jobs.  
It uses the **SimpleShell** from the previous assignment, allowing users to interact with the scheduler by submitting jobs that the scheduler manages and executes.

---

## Components

| File | Description |
|------|--------------|
| **scheduler.c** | Implements the SimpleScheduler daemon. Manages process queues, handles `SIGALRM` timer interrupts, and performs round-robin scheduling across multiple CPUs. |
| **scheduler.h** | Header file exposing the scheduler API (`initialise_scheduler`, `submit_process`, `stop_scheduler`, `print_stats`, etc.). |
| **shell.c** | Implements **SimpleShell**, an interactive shell to submit, manage, and exit jobs. |
| **job.c** | A sample user job demonstrating compatibility with the scheduler. Contains CPU-bound work to test preemption. |
| **dummy_main.h** | Header wrapper for user programs. Intercepts the real `main()` and delays job start until scheduled. |
| **Makefile** | Automates the build process for all components. |

---

## Scheduling Mechanism

**SimpleScheduler** maintains:

- A ready queue for processes waiting to run.  
- A master process list preserving all process statistics.

At every time quantum (**TSLICE**):

- Running processes receive `SIGSTOP` and are requeued.  
- Scheduler dispatches up to **NCPU** new processes using `SIGUSR1` or `SIGCONT`.  
- Terminated processes are detected using `waitpid(WNOHANG)` and marked complete.  

This ensures **fairness across all jobs** using the **Round-Robin** policy.

---

## Termination

When the user types `exit` or presses **Ctrl+C**, the shell:

- Stops the scheduler timer.  
- Waits for all jobs to finish.  
- Prints final statistics (completion and waiting time).

---

## Implementation Details

### Signals Used
- `SIGALRM` – triggers scheduler timer (handled by `roundrobin_alarm_handler`)  
- `SIGSTOP` / `SIGCONT` – used to pause and resume processes  
- `SIGUSR1` – used once to start a job’s first execution  

### Timer Setup
- Implemented using `setitimer(ITIMER_REAL, ...)`  
- Periodically invokes the scheduler every **TSLICE** milliseconds  

### Data Structures
- **`ProcessInternal` struct** – stores PID, priority, queue links, and accounting info  
- **Linked lists** – used for the ready queue and master process list  

---

## Statistics Report

At termination, **SimpleShell** prints:

- **Job name**  
- **PID**  
- **Completion time** (in multiples of TSLICE)  
- **Wait time** (total slices spent waiting in queue)

All statistics are derived from the **master process list**, which preserves information across preemptions.

---

## Contribution

- **Agrim Verma** –  
- **Rohan Singh** –  

---

## Compilation and Execution

To compile and run the program:

```bash
make
./SimpleShell <NCPU> <TSLICE_MS>
```
---

## Github repo link: https://github.com/agrim-00/OS_SimpleScheduler
