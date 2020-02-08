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
     *  Condition
     */
    int waiting;
    int echo;
    cond_id_t echoSig;
    cond_id_t ccl;

    cond_id_t ownThread;
    int activeWriting;
};

/**
 * Condition that terminal zero has been initiated
 */
static int tzInit;

/**
 * Condition that multiple terminals are connected to the monitor
 */
static int multT;

/**
 * Terminal Zero, the first terminal to conenct to the monitor
 */
struct terminal termZero;

/**
 * Array of terminal info objects
 */
static struct terminal terminals[NUM_TERMINALS];

/**
 * Entry index for next terminal
 */
static int termIn = 0;

static cond_id_t writing;

static int termStates[NUM_TERMINALS];

/**
 * Returns information about @term
 */
struct terminal getTerminal(int term) {
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
    return terminals[t];
}



void echoToTerminal(int term, struct terminal t, char c) {
    if(t.inputBuf.out - t.inputBuf.in == BUFFER_SIZE) {
        return;
    }
    else {
        CondWait(writing);
        WriteDataRegister(term,c);
    }
}

extern void ReceiveInterrupt(int term) {
    Declare_Monitor_Entry_Procedure();

    struct terminal t = getTerminal(term);
    if(fullBuf(t.inputBuf)) {
        return;
    }
    char c = ReadDataRegister(term);
    printf("%c in buffer",c);
    switch(c){
        case '\r': pushToBuffer(t.inputBuf,'\n');
            echoToTerminal(term,t,'\r');
            echoToTerminal(term,t,'\n');
            break;
        case '\n':  pushToBuffer(t.inputBuf, '\n');
            echoToTerminal(term,t,'\r');
            echoToTerminal(term,t,'\n');
            break;
        case '\b': pullFromBuffer(t.inputBuf);
                echoToTerminal(term,t,'\b');
                break;
        case '\177': pullFromBuffer(t.inputBuf);
                echoToTerminal(term,t,'\b');
                break;
        default:
            pushToBuffer(t.inputBuf, c);
            echoToTerminal(term,t,c);
}}

extern void TransmitInterrupt(int term) {
    Declare_Monitor_Entry_Procedure();
    struct terminal t = getTerminal(term);
    CondSignal(writing);

    //if(t.nextTerm.writing)
    //CondSignal(t.ccl);
}

extern int WriteTerminal(int term, char* buf, int buflen) {

    Declare_Monitor_Entry_Procedure();
    struct terminal t = getTerminal(term);
    //if(t.activeWriting) {
//        CondWait(t.ownThread);
  //  }
    printf("%d",termStates[term]);
    //t.activeWriting = 1;
    termStates[term] = 1;
    printf("%d",termStates[term]);



    //t.writing = 1;
    if(buflen == 0) return 0;
    int c;
    for(c = 0; c < buflen; c++) {
        char next = buf[c];
        if(next == '\n') {
            WriteDataRegister(term,'\r');
            CondWait(writing);
        }
        WriteDataRegister(term,next);
        CondWait(writing);

    }

    //t.activeWriting = 0;
    CondSignal(t.ownThread);
    return c;
}

extern int ReadTerminal(int term, char* buf, int buflen) {
    Declare_Monitor_Entry_Procedure();
    if(buflen == 0) return 0;
    int c = 0;
    struct terminal t = getTerminal(term);
    char next;
    while(c < buflen) {
        next = popFromBuffer(t.inputBuf);
        if(next == '\n') {
            c++;
            break;
        }
        buf[c] = 'h';
        c++;
    }
    return c;
}

extern int InitTerminal(int term) {
    Declare_Monitor_Entry_Procedure();

    struct terminal t;
    t.term = term;
    t.inputBuf.in = 0;
    t.inputBuf.out = NULL;
    t.inputBuf.b = malloc(sizeof(char)*1000);
    //t.writeCond = CondCreate();
    t.activeWriting = 2;
    t.ccl = CondCreate();
    t.echoSig = CondCreate();
    t.ownThread = CondCreate();
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
    writing = CondCreate();
    tzInit = 0;
    multT = 0;
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

static void pushToBuffer(struct buffer *buf, char c) {
    if(buf->full) {
        printf("Full Buf\n");
        return;
    }
    if(buf->empty) {
        buf->empty--;
    }
    buf->b[buf->in] = c;
    buf->in++;
    if(buf->in >= buf->bufferSize) {
        buf->in = 0;
    }
    if(buf->in == buf->out) {
        buf->full = 1;
    }

}

static char popFromBuffer(struct buffer *buf) {
    if(buf->empty) {
        return '\0';
    }
    char next = buf->b[buf->out];
    buf->out++;
    if(buf->out >= buf->bufferSize) {
        buf->out = 0;
    }
    if(buf->full) {
        buf->full--;
    }
    return next;
}