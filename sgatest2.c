// Simply serves as an echo terminal to test the driver.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <terminals.h>
#include <signal.h>
#include <unistd.h>

#define EXITMSG "Exiting terminal :)\n"

void setupGracefulExit();
void echoterm(void *);

int main(int argc, char **argv) {
    InitTerminalDriver();
    InitTerminal(1);
    setupGracefulExit();

    if (argc > 1) HardwareOutputSpeed(1, atoi(argv[1]));
    if (argc > 2) HardwareInputSpeed(1, atoi(argv[2]));

    ThreadCreate(echoterm, NULL);
    ThreadWaitAll();

    exit(0);
}

void echoterm(void * arg) {
    char *buf = calloc(400, 1);
    char generic[] = "Input Buffer Says: ";
    char generic2[] = "You Say: ";
    char *newline;

    while (1) {
        WriteTerminal(1, generic2, 9);
        ReadTerminal(1, buf, 400);
        newline = memchr(buf, '\n', 400);
        WriteTerminal(1, generic, 19);
        WriteTerminal(1, buf, newline - buf + 1);
    }
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