//
// Created by corne on 2/15/2025.
//comments for oss




#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>



/*
 * OSS Process: Managing Worker Processes
 *
 * The task of `oss` is to launch a certain number of worker processes with specific parameters.
 * These parameters are determined by its own command-line arguments.
 *
 * Invocation:
 *      oss [-h] [-n proc] [-s simul] [-t timelimitForChildren] [-i intervalInMsToLaunchChildren]
 *
 * ### Parameter Breakdown:
 * - `-n proc`      : Specifies the number of worker processes to be launched.
 * - `-s simul`     : Defines the maximum number of worker processes that can run simultaneously.
 * - `-t timelimitForChildren` : Specifies the **upper bound** of time a worker process will run for.
 *   - Example: If `-t 7` is used, each worker process will have a lifespan randomly chosen between
 *     **1 second and 7 seconds**, with nanoseconds also randomized.
 * - `-i intervalInMsToLaunchChildren` : Specifies how often a new child process is launched.
 *   - Example: If `-i 100` is passed, a child will be launched at most **once every 100 milliseconds**
 *     (based on the system clock). This prevents all processes from flooding the system at once.
 *
 * ### Process Flow:
 * 1. `oss` initializes the system clock.
 * 2. It enters a loop where it:
 *    - Uses `fork()` and `exec()` to create worker processes.
 *    - Ensures that no more than `simul` worker processes are running at any given time.
 *    - Updates the process table as it launches new processes.
 * 3. Unlike previous implementations, `oss` **does not** use `wait()` calls for process termination.
 * 4. Instead, it continuously:
 *    - Increments the system clock.
 *    - Checks if any child process has terminated.
 *
 * The rough pseudocode for this loop is outlined below:
 */



#define SHM_KEY 859047  // Fixed shared memory key

// Define the shared memory structure for system clock
struct SystemClock {
    int seconds;
    int nanoseconds;
};

void usage() {
    fprintf(stderr, "Help: oss [-h] [-n proc] [-s simul] [-t timelimitForChildren] [-i intervalInMsToLaunchChildren]\n");
    exit(1);
}

void commandLine(int argc, char *argv[], int *n, int *s, int *t, int *i) {
    int c;
    while ((c = getopt(argc, argv, "hn:s:t:i:")) != -1) {
        switch (c) {
            case 'h':
                usage();
                break;
            case 'n':
                *n = atoi(optarg);
                break;
            case 's':
                *s = atoi(optarg);
                break;
            case 't':
                *t = atoi(optarg);
                break;
            case 'i':
                *i = atoi(optarg);
                break;
            default:
                usage();
        }
    }
}

int main(int argc, char *argv[]) {
    int n = 0, s = 0, t = 0, i = 0;
    commandLine(argc, argv, &n, &s, &t, &i);
    if (n == 0 || s == 0 || t == 0 || i == 0) {
        usage();
    }

    printf("n = %d, s = %d, t = %d, i = %d\n", n, s, t, i);

    // Create shared memory for the system clock
    int shmid = shmget(SHM_KEY, sizeof(struct SystemClock), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach to shared memory
    struct SystemClock *sysClock = (struct SystemClock *) shmat(shmid, NULL, 0);
    if (sysClock == (void *) -1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize system clock
    sysClock->seconds = 0;
    sysClock->nanoseconds = 0;

    printf("Shared Memory Created: shmid = %d\n", shmid);

    // Fork worker processes
    int active_processes = 0;
    for (int j = 0; j < n; j++) {
        if (active_processes < s) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                exit(1);
            } else if (pid == 0) {
                // Child process
                char shmid_str[10], duration_str[10];
                sprintf(shmid_str, "%d", shmid);
                sprintf(duration_str, "%d", t);  // Worker receives `t` (duration)

                execl("./worker", "worker", shmid_str, duration_str, (char *) NULL);
                perror("execl failed");
                exit(1);
            } else {
                // Parent process
                active_processes++;
            }
        }
    }

    // Wait for all child processes to terminate
    for (int j = 0; j < n; j++) {
        wait(NULL);
    }

    // Cleanup shared memory
    shmdt(sysClock);
    shmctl(shmid, IPC_RMID, NULL);
    printf("Shared Memory Removed: shmid = %d\n", shmid);

    return 0;
}