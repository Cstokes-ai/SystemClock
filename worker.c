//
// Created by corne on 2/15/2025.
//
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
// I think I will start on this file first

/* Worker Process: Managing Child Processes

The worker takes in two command line arguments, corresponding to the maximum time it should stay active in the system.
For example, if you were running it directly, you might call it like:

    ./worker 5 500000

The worker will attach to shared memory and examine the simulated system clock. It will then determine its termination time
by adding the provided duration (in simulated system time, not actual time) to the system clock's current value.

### Example:
If the system clock is showing **6 seconds and 100 nanoseconds** and the worker is passed **5 seconds and 500000 nanoseconds**,
the target termination time would be:

    11 seconds and 500100 nanoseconds

The worker will then enter a loop, continuously checking the system clock until the termination time has passed. If at any
point it detects that the system time has exceeded this value, it should print a message and terminate.

### Output Format:
Upon startup, the worker should print:

    WORKER PID:6577 PPID:6576 SysClockS: 5 SysclockNano: 1000 TermTimeS: 11 TermTimeNano: 500100
    --Just Starting

The worker should then enter a loop, checking for its expiration time **without using sleep**. It should also produce periodic
output whenever it notices a second has passed:

    WORKER PID:6577 PPID:6576 SysClockS: 6 SysclockNano: 45000000 TermTimeS: 11 TermTimeNano: 500100
    --1 second has passed since starting

    WORKER PID:6577 PPID:6576 SysClockS: 7 SysclockNano: 500000 TermTimeS: 11 TermTimeNano: 500100
    --2 seconds have passed since starting

Once its time has elapsed, it should output a final message before terminating:

    WORKER PID:6577 PPID:6576 SysClockS: 11 SysclockNano: 700000 TermTimeS: 11 TermTimeNano: 500100
    --Terminating
*/




// The worker taake in two command line arguments. Corresponding to the maximum time.
//All system clock stuff is in PCB structure.  I need to have this output
/*WORKER PID:6577 PPID:6576 SysClockS: 5 SysclockNano: 1000 TermTimeS: 11 TermTimeNano: 500100
--Just Starting
The worker should then go into a loop, checking for its time to expire (IT SHOULD NOT DO A SLEEP). It should also do
some periodic output. Every time it notices that the seconds have changed, it should output a message like:
WORKER PID:6577 PPID:6576 SysClockS: 6 SysclockNano: 45000000 TermTimeS: 11 TermTimeNano: 500100
--1 seconds have passed since starting
and then one second later it would output:
WORKER PID:6577 PPID:6576 SysClockS: 7 SysclockNano: 500000 TermTimeS: 11 TermTimeNano: 500100
--2 seconds have passed since starting
Once its time has elapsed, it would send out one ﬁnal message:
WORKER PID:6577 PPID:6576 SysClockS: 11 SysclockNano: 700000 TermTimeS: 11 TermTimeNano: 500100
--Terminating
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>

#define SHM_KEY 859047  // Must match oss.c

struct SystemClock {
    int seconds;
    int nanoseconds;
};

void usage() {
    fprintf(stderr, "Usage: worker <shmid> <duration>\n");
    exit(1);
}

void worker(int shmid, int duration) {
    // Attach to shared memory
    struct SystemClock *sysClock = (struct SystemClock *) shmat(shmid, NULL, 0);
    if (sysClock == (void *) -1) {
        perror("shmat failed");
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }

    printf("WORKER PID: %d attached to shared memory ID: %d\n", getpid(), shmid);

    int startSeconds = sysClock->seconds;
    int startNano = sysClock->nanoseconds;

    int termTimeS = startSeconds + (duration / 1000000000);
    int termTimeNano = startNano + (duration % 1000000000);
    if (termTimeNano >= 1000000000) {
        termTimeS++;
        termTimeNano -= 1000000000;
    }

    printf("WORKER PID: %d -- Just Starting\n", getpid());

    while (1) {
        if (sysClock->seconds > termTimeS || (sysClock->seconds == termTimeS && sysClock->nanoseconds > termTimeNano)) {
            printf("WORKER PID: %d -- Terminating\n", getpid());
            break;
        }
    }

    shmdt(sysClock);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage();
    }

    int shmid = atoi(argv[1]);
    int duration = atoi(argv[2]);

    if (shmid <= 0) {
        fprintf(stderr, "Error: Invalid shared memory ID\n");
        exit(1);
    }

    if (duration <= 0) {
        fprintf(stderr, "Error: Invalid duration\n");
        exit(1);
    }

    worker(shmid, duration);
    return 0;
}