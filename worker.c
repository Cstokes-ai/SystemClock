#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

// Shared memory structure for the simulated clock
typedef struct {
    int seconds;
    int nanoseconds;
} SharedClock;

void run_worker(int maxSec, int maxNano) {
    // Attach to the shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedClock), 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    SharedClock *simClock = (SharedClock *)shmat(shmid, NULL, 0);
    if (simClock == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    // Calculate the termination time
    int termSec = simClock->seconds + maxSec;
    int termNano = simClock->nanoseconds + maxNano;
    if (termNano >= 1000000000) {
        termSec++;
        termNano -= 1000000000;
    }

    // Initial output
    printf("WORKER PID:%d PPID:%d SysClock:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d\n",
           getpid(), getppid(), simClock->seconds, simClock->nanoseconds, termSec, termNano);

    // Loop until the termination time is reached
    int lastSecond = simClock->seconds;
    while (simClock->seconds < termSec || (simClock->seconds == termSec && simClock->nanoseconds < termNano)) {
        if (simClock->seconds != lastSecond) {
            printf("WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d --%d seconds passed\n",
                   getpid(), getppid(), simClock->seconds, simClock->nanoseconds, termSec, termNano, simClock->seconds - lastSecond);
            lastSecond = simClock->seconds;
        }
    }

    // Final output
    printf("WORKER PID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d --Terminating\n",
           getpid(), simClock->seconds, simClock->nanoseconds, termSec, termNano);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <maxSeconds> <maxNanoseconds>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int maxSec = atoi(argv[1]);
    int maxNano = atoi(argv[2]);
    if (maxSec < 0 || maxNano < 0) {
        fprintf(stderr, "Error: maxSeconds and maxNanoseconds must be non-negative integers.\n");
        return EXIT_FAILURE;
    }

    run_worker(maxSec, maxNano);
    return EXIT_SUCCESS;
}
