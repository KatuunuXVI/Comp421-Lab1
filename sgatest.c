/**
 * Simply opens terminal for writing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <terminals.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define EXITMSG "Exiting terminal :)\n"

void setupGracefulExit();
void openTerm(void *);

int main(int argc, char **argv) {
    InitTerminalDriver();
    InitTerminal(1);
    setupGracefulExit();

    if (argc > 1) HardwareOutputSpeed(1, atoi(argv[1]));
    if (argc > 2) HardwareInputSpeed(1, atoi(argv[2]));

    ThreadCreate(openTerm, NULL);

    ThreadWaitAll();

    exit(0);
}

void openTerm(void *arg) {
    while (1) ;
}

/* Handle exiting gracefully. */

void sigAbrtHandler(int signum, siginfo_t *info, void *ptr) {
    write(STDOUT_FILENO, EXITMSG, sizeof(EXITMSG));
    exit(0);
}

void catchSigTerm(void *arg) {
    static struct sigaction _sigact;
    memset(&_sigact, 0, sizeof(_sigact));
    _sigact.sa_sigaction = sigAbrtHandler;
    _sigact.sa_flags = SA_SIGINFO;

    sigaction(SIGABRT, &_sigact, NULL);
}

void setupGracefulExit() {
    ThreadCreate(catchSigTerm, NULL);
}