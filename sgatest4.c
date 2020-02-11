// Annoying program that prints what you write to it a lot.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <terminals.h>
#include <signal.h>
#include <unistd.h>

#define EXITMSG "Exiting terminal :)\n"

void setupGracefulExit();

void annoyterm(void *);
void readterm(void *);

static char *buf;

int main(int argc, char **argv) {
    InitTerminalDriver();
    InitTerminal(1);
    setupGracefulExit();

    // Get hardware arguments.
    if (argc > 1) HardwareOutputSpeed(1, atoi(argv[1]));
    if (argc > 2) HardwareInputSpeed(1, atoi(argv[2]));

    // Make thread and run.
    buf = calloc(400, 1);
    *buf = ':';
    *(buf + 1) = ')';
    *(buf + 2) = ' ';
    *(buf + 3) = '\n';
    ThreadCreate(annoyterm, NULL);
    ThreadCreate(readterm, NULL);
    ThreadWaitAll();

    exit(0);
}

void annoyterm(void * arg) {
    char *newline;
    while (1) {
        newline = memchr(buf, '\n', 400);
        WriteTerminal(1, buf, (newline - buf) >= 0 ? (newline - buf) : 0);
    }
}

void readterm(void * arg) {
    char *other_buf = calloc(400, 1);
    while (1) {
        ReadTerminal(1, other_buf, 400);
        buf = other_buf;
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
