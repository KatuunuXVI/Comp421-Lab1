// Functional chatting application.
// Can support up to 4 terminals!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <terminals.h>
#include <signal.h>
#include <unistd.h>

#define EXITMSG "Exiting terminal :)\n"

void setupGracefulExit();

struct term_input {
    int termid;
    char *buf1;
    char *buf2;
    int buf2size;
};

void chatterm(void *);
void writeToTerm(void *);
static int num_users;

int main(int argc, char **argv) {
    InitTerminalDriver();
    setupGracefulExit();

    // Get number of users and other arguments.
    num_users = 3;
    if (argc > 1) num_users = atoi(argv[1]);
    if (argc > 2) HardwareOutputSpeed(1, atoi(argv[2]));
    if (argc > 3) HardwareInputSpeed(1, atoi(argv[3]));

    if (num_users > 4 || num_users < 1) {
        fprintf(stderr, "Inputted invalid number of users. Max is 4.\n");
        return -1;
    }

    // Initialize terminals for users.
    int i;
    for (i = 0; i < num_users; i++) {
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        InitTerminal(i);
        ThreadCreate(chatterm, arg);
    }

    // Finish.
    ThreadWaitAll();
    exit(0);
}

void chatterm(void *arg) {
    // Get number of terminal.
    int term = *((int *) arg);

    // Initialize input string and output values.
    char *input = calloc(400, 1);
    char start[] = "User";
    char end[] = "Says:";
    char declaration[14];
    sprintf(declaration, "%s %d %s ", start, term + 1, end);

    // Make input struct for each terminal.
    struct term_input **terms = calloc(num_users, sizeof(void *));
    struct term_input **pos = terms;
    int i;
    for (i = 0; i < num_users; i++) {
        if (i == term) {
            pos++;
            continue;
        }

        *pos = malloc(sizeof(struct term_input));
        (*pos)->termid = i;
        pos++;
    }

    // Start communicating.
    char* newline;
    while (1) {
        // Determine next input line.
        ReadTerminal(term, input, 400);
        newline = memchr(input, '\n', 400);

        // Write to all other terminals.
        for (i = 0; i < num_users; i++) {
            if (i == term)
                continue;

            // Make structure.
            terms[i]->buf1 = declaration;
            terms[i]->buf2 = input;
            terms[i]->buf2size = newline - input + 1;

            // Start thread.
            ThreadCreate(writeToTerm, terms[i]);
        }
    }
}

void writeToTerm(void *arg) {
    struct term_input *val = (struct term_input *) arg;
    WriteTerminal(val->termid, val->buf1, 13);
    WriteTerminal(val->termid, val->buf2, val->buf2size);
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