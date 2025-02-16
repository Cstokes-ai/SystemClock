//
// Created by corne on 2/15/2025.
//
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
//Comments for worker

struct PCB{
    int occupied; // either true or false
    pid_t pid; // process id of this child
    int startSeconds; // time when it was forked
    int startNano; //time when it was forked

};



void user(int n) {  // Accepts n as a parameter
    for (int i = 0; i < n; i++) {

        printf("User ID: %d Parent ID: %d Iteration: %d Before Sleeping\n", getpid(), getppid(), i);
        sleep(1);
        printf("User ID: %d Parent ID: %d Iteration: %d After Sleeping\n", getpid(), getppid(), i);
    }
}

int main(int argc, char *argv[]) {

    int n = atoi(argv[1]);
    // Convert argument to integer
    user(n);  // Call user function with n iterations
    return 0;
}