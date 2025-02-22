#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

//shared memory for the simulated clock
typedef struct {
    int seconds;
    int nanoseconds;
} SharedClock;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <maxSeconds> <maxNanoseconds>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int maxSec = atoi(argv[1]);
    int maxNano = atoi(argv[2]);

    //attach to the shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedClock), 0666);
    SharedClock *simClock = (SharedClock *)shmat(shmid, NULL, 0);

    //calculate the time to terminate
    int termSec = simClock->seconds + maxSec;
    int termNano = simClock->nanoseconds + maxNano;
    if (termNano >= 1000000000) {
        termSec++;
        termNano -= 1000000000;
    }

    //initial output
    printf("WORKER PID:%d PPID:%d SysClock:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d --%d seconds passed since starting\n", getpid(), simClock->seconds, simClock->nanoseconds, termSec, termNano);

    //loop until the termination time is reached
    while (simClock->seconds < termSec || (simClock->seconds == termSec && simClock->nanoseconds < termNano)) {
        //output every time the seconds change
        static int lastSecond = 0;
        if (simClock->seconds != lastSecond) {
            printf("WORKER PID:%d PPID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d --%d seconds passed since starting\n", getpid(), getppid(), simClock->seconds, simClock->nanoseconds, termSec, termNano, simClock->seconds - lastSecond);
            lastSecond = simClock->seconds;
        }
    }

    //final output
    printf("WORKER PID:%d SysClockS:%d SysClockNano:%d TermTimeS:%d TermTimeNano:%d --Terminating\n", getpid(), getppid(), simClock->seconds, simClock->nanoseconds, termSec, termNano);

    return 0;
}
