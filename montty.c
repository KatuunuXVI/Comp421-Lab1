//TODO Change "" to <> before submitting, Make sure to update periodically
#include <hardware.h>
#include <terminals.h>
#include <threads.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>


#define BUFFER_SIZE 10000


/**
 * Struct for a Character Buffer
 */
struct buffer {
    /**
     * Size to go to until the buffer loops
     */
    int bufferSize;
    /**
     * Pointer to character array
     */
    char *b;
    /**
     * First filled space, next to be removed
     */
    int out;

    /**
     * First empty space, next to be filled
     */
    int in;
    /**
     * Condition on if the buffer is empty
     */
    int empty;
    /**
     * Condition on if the buffer is full
     */
    int full;
};

static struct buffer getBuffer(int size);

static void pushToBuffer(struct buffer *buf, char c);
static void pullFromBuffer(struct buffer *buf);
static char popFromBuffer(struct buffer *buf);

struct terminal {
    /**
     * Term number for terminal object
     */
    int term;
    /**
     * Input buffer for reading keystrokes
     */
    struct buffer inputBuf;

    /**
     * Buffer for visualizing input
     */
    struct buffer echoBuf;

    /**
     *  Condition
     */
    int waiting;

    /**
     * Condition for echoing
     */
    int echo;
    /**
     * Signal for echoing to the terminal. If echo buffer isn't empty this must be signaled before read
     */
     cond_id_t echoSignal;
    /**
     *
     */
     int reading;
    /**
     * Signal for a specific ReadTerminal Thread
     */
     cond_id_t readSignal;

     /**
      * Signal to waiting ReadTerminal Thread for Terminal
      */
    cond_id_t readThread;
};

/**
 * Array of terminal info objects
 */
static struct terminal terminals[NUM_TERMINALS];

/**
 * Entry index for next terminal
 */
static int termIn = 0;

static int writing = 0;

static cond_id_t writingSig;

static cond_id_t echoSig;

static int termStates[NUM_TERMINALS];


/**
 * Returns information about @term
 */
struct terminal *getTerminal(int term) {
    int t = 0;
    bool found = false;
    /**
     * Iterates through the index
     */
    while(t < NUM_TERMINALS) {
        if(terminals[t].term == term) {
            found = true;
            break;
        }
        t++;
    }
    /**
     * Program aborts if terminal is not Found
     * DO NOT CALL UNLESS THERE IS A TERMINAL INITIALIZED
     */
    if(!found) {
        exit(-1);
    } 
    return &terminals[t];
}

int checkEcho() {
    int t = 0;
    while(t < NUM_TERMINALS) {
        if(terminals[t].echo) {
            return 1;
        }
        t++;
    }
    return 0;
}


void echoToTerminal(int term, struct terminal *t) {
    t->echo = 1;
    if(t->inputBuf.full) {
        return;
    }
    else {
        if(writing) {CondWait(t->echoSignal);}
        WriteDataRegister(term,popFromBuffer(&t->echoBuf));
        t->echo = 0;
    }
}

extern void ReceiveInterrupt(int term) {
    Declare_Monitor_Entry_Procedure();
    struct terminal *t = getTerminal(term);
    if(t->inputBuf.full) {
        return;
    }

    int empty = t->inputBuf.empty;
    char c = ReadDataRegister(term);
    switch(c) {
        case '\r':
            pushToBuffer(&t->inputBuf, '\n');
            pushToBuffer(&t->echoBuf, '\r');
            pushToBuffer(&t->echoBuf, '\n');
            break;
        case '\n':
            pushToBuffer(&t->inputBuf, '\n');
            pushToBuffer(&t->echoBuf, '\r');
            pushToBuffer(&t->echoBuf, '\n');
            break;
        case '\b':
            pullFromBuffer(&t->inputBuf);
            pushToBuffer(&t->inputBuf, '\b');
            pushToBuffer(&t->echoBuf, '\b');
            break;
        case '\177':
            pullFromBuffer(&t->inputBuf);
            pushToBuffer(&t->inputBuf, '\b');
            pushToBuffer(&t->echoBuf, '\b');
            break;
        default:
            pushToBuffer(&t->inputBuf, c);
            pushToBuffer(&t->echoBuf, c);
    }

    /**
     * Begin echo process if not already started
     */
    if(!(t->echo)) {echoToTerminal(term, t);}
    /**
     * Alert the ReadTerminal Routine that it can continue
     */
    if(empty && t->reading) {CondSignal(t->readSignal);}


}

extern void TransmitInterrupt(int term) {
    Declare_Monitor_Entry_Procedure();
    struct terminal *t = getTerminal(term);
    if(t->echo) {
        CondSignal(t->echoSignal);
    } else if(!(t->echoBuf.empty)) {
        echoToTerminal(term,t);
    }
    else if(checkEcho()) {
      CondSignal(echoSig);
    } else {
        CondSignal(writingSig);
    }

}

extern int WriteTerminal(int term, char* buf, int buflen) {

    Declare_Monitor_Entry_Procedure();

    if(buflen == 0) return 0;
    struct terminal *t = getTerminal(term);
    int c;
    if(writing || checkEcho()) {
        CondWait(writingSig);
    }
    writing += 1;
    for(c = 0; c < buflen; c++) {
        char next = buf[c];
        WriteDataRegister(term,next);
        CondWait(writingSig);
        if(next == '\n') {
            WriteDataRegister(term,'\r');
            CondWait(writingSig);
        }
    }
    CondSignal(writingSig);
    writing -= 1;
    return c;
}

extern int ReadTerminal(int term, char* buf, int buflen) {
    Declare_Monitor_Entry_Procedure();
    if(buflen == 0) return 0;
    struct terminal *t = getTerminal(term);
    if(t->reading) {
        CondWait(t->readThread);
    }
    t->reading = 1;
    int c = 0;
    char next;
    while(c < buflen) {
        next = popFromBuffer(&t->inputBuf);
        if(next == '\0') {
            CondWait(t->readSignal);
            continue;
        }
        if(next == '\b') {
            c--;
            if(c < 0) {c = 0;}
            continue;
        }
        buf[c] = next;
        c++;
        if(next == '\n') {
            break;
        }
    }
    t->reading = 0;
    CondSignal(t->readThread);
    return c;
}

extern int InitTerminal(int term) {
    Declare_Monitor_Entry_Procedure();
    struct terminal t;
    t.term = term;
    t.inputBuf = getBuffer(1000);
    t.echoBuf = getBuffer(1000);
    t.echo = 0;
    t.reading = 0;
    t.readSignal = CondCreate();
    t.readThread = CondCreate();
    t.echoSignal = CondCreate();
    //t.activeWriting = 2;
    terminals[termIn] = t;
    termStates[term] = 0;
    /*if(!tzInit) {
        termZero = t;
        tzInit = 1;
        t.nextTerm = t;
        t.prevTerm = t;
    } else {
        multT = 1;
        t.nextTerm = termZero;
        t.prevTerm = terminals[termIn-1];
    }*/
    termIn ++;
    return InitHardware(term);
}

extern int TerminalDriverStatistics(struct termstat *stats) {
    Declare_Monitor_Entry_Procedure();
    return 0;
}

extern int InitTerminalDriver() {
    Declare_Monitor_Entry_Procedure();
    writingSig = CondCreate();
    echoSig = CondCreate();
    return 0;
}

/**
 * Buffer Routines
 */

static struct buffer getBuffer(int size) {
    struct buffer newBuf;
    newBuf.bufferSize = size;
    newBuf.b = malloc(sizeof(char) * size);
    newBuf.in = 0;
    newBuf.out = 0;
    newBuf.empty = 1;
    newBuf.full = 0;
    return newBuf;
}

static void pullFromBuffer(struct buffer *buf) {
    if(buf->empty) {
        return;
    } if(buf->full) {
        buf->full = 0;
    }
    buf->in--;
    if(buf->in < 0) {
        buf->in = buf->bufferSize-1;
    }
    buf->b[buf->in] ='\0';
    if(buf->in == buf->out) {
        buf->empty = 1;
    }
}

static void pushToBuffer(struct buffer *buf, char c) {
    if(buf->full) {
        return;
    }
    if(buf->empty) {
        buf->empty = 0;
    }
    buf->b[buf->in] = c;
    buf->in++;
    if(buf->in >= buf->bufferSize) {
        buf->in = 0;
    }
    if(buf->in == buf->out) {
        buf->full = 1;

}}

static char popFromBuffer(struct buffer *buf) {
    if(buf->empty) {
        return '\0';
    }
    char next = buf->b[buf->out];
    buf->out++;
    if(buf->out >= buf->bufferSize) {
        buf->out = 0;
    }
    if(buf->out == buf->in) {
        buf->empty = 1;
    }
    if(buf->full) {
        buf->full--;
    }
    return next;
}