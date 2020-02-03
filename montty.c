//TODO Change "" to <> before submitting, Make sure to update periodically
#include <hardware.h>
#include <terminals.h>
#include <threads.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>


#define BUFFER_SIZE 10000



static int echo = 0;



struct buffer {
    char *b;
    char nextConsumed;
    int out;
    int in;
};

static int fullBuf(struct buffer buf);

struct terminal {
    int term;
    struct buffer inputBuf;
    struct buffer echoBuf;
    int prevTerm;
    int nextTerm;
    int writing;
    int echo;
    cond_id_t ccl;
};

static int tzInit = 0;

struct terminal termZero;

static struct terminal terminals[NUM_TERMINALS];

static int termIn = 0;

struct terminal getTerminal(int term) {
    int t = 0;
    bool found = false;
    while(t < NUM_TERMINALS) {
        if(terminals[t].term == term) {
            found = true;
            break;
        }
        t++;
    }
    if(!found) {
        exit(-1);
    } 
    return terminals[t];
}


void pushToBuffer(struct buffer buf, char c) {
    if(buf.in - buf.out == BUFFER_SIZE) {
        return;
    }
    buf.b[buf.in%BUFFER_SIZE] = c;
    buf.in++;
    if(buf.in > BUFFER_SIZE -1) {
        buf.in = 0;
    }
}

char popFromBuffer(struct buffer buf) {
    if(buf.in == buf.out) {
        return '\0';
    }
    char next = buf.b[buf.out];
    buf.out++;
    if(buf.out > BUFFER_SIZE -1) {
        buf.out = 0;
    }
    return next;
}

void pullFromBuffer(struct buffer buf) {
    if(buf.in == buf.out) {
        return;
    }
    buf.in --;
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
        default: pushToBuffer(t.inputBuf, c);
            echoToTerminal(term,t,c);
}}

extern void TransmitInterrupt(int term) {
    Declare_Monitor_Entry_Procedure();
    struct terminal t = getTerminal(term);
    CondSignal(writing);
}

extern int WriteTerminal(int term, char* buf, int buflen) {
    Declare_Monitor_Entry_Procedure();
    struct terminal t = getTerminal(term);
    if(buflen == 0) return 0;
    int c;
    for(c = 0; c < buflen; c++) {
        if(echo) {
            WriteDataRegister(term, 'O');
            CondWait(writing);
        }
        char next = buf[c];
        if(next == '\n') {
            WriteDataRegister(term,'\r');
            CondWait(writing);
        }
        WriteDataRegister(term,next);
        CondWait(writing);

    }
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
            break;
        }
        buf[c] = next;
        c++;
    }
    return 0;
}

extern int InitTerminal(int term) {
    Declare_Monitor_Entry_Procedure();
    struct terminal t;
    t.term = term;
    t.inputBuf.in = 0;
    t.inputBuf.out = 0;
    t.inputBuf.b = malloc(sizeof(char)*1000);
    t.echoBuf.in = 0;
    t.echoBuf.out = 0;
    t.echoBuf.b = malloc(sizeof(char)*1000);
    //t.writeCond = CondCreate();
    terminals[termIn] = t;
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
    return 0;
}

static int emptyBuf(struct buffer buf) {
    return buf.in == buf.out;
}

static int fullBuf(struct buffer buf) {
    return ((buf.out+1) - buf.in) == BUFFER_SIZE;
}