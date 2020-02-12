//TODO Change "" to <> before submitting, Make sure to update periodically
#include <hardware.h>
#include <terminals.h>
#include <threads.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

/**
 * Buffer Size for inputs
 */
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

/**
 * Constructor message for a new buffer
 * @param size How much space to allocate for the buffer
 */
static struct buffer getBuffer(int size);

/**
 * Adds a value to the buffers queue
 * @param buf Buffer to add to
 * @param c Character that is being added
 */
static void pushToBuffer(struct buffer *buf, char c);
/**
 * Removes a value from the tail end of a buffer
 * @param buf Buffer to remove from
 */
static void pullFromBuffer(struct buffer *buf);
/**
 * Pops the next character from the buffer's queue
 * @param buf Buffer requesting character from
 * @return the character that was popped
 */
static char popFromBuffer(struct buffer *buf);

static void incrementBuffer(struct buffer *buf, int increments);

/**
 * Struct containing information about a given terminal
 */
struct terminal {

    /**
     * Term number for terminal object
     */
    int term;

    /**
     * Input buffer for reading keystrokes
     */
    struct buffer inputBuf;

    struct buffer readBuf;
    /**
     * Condition measuring how many read processes this terminal is going through
     */
    int reading;

    /**
     * Measures the Data Reads made by this terminal
     */
     int tty_in;

    /**
     * Measures the Data Writes made by this terminal
     */
    int tty_out;

    /**
     * Measures the Writes to the terminal
     */
     int user_in;

     /**
      * Measures the reading of the terminal
      */
      int user_out;
    /**
     * Signal for a specific ReadTerminal Thread
     */
    cond_id_t readSignal;

    /**
     * Signal to waiting ReadTerminal Thread for Terminal
     */
    cond_id_t readThread;
};

static int terms[NUM_TERMINALS];

/**
 * Array of terminal info objects
 */
static struct terminal terminals[NUM_TERMINALS];

/**
 * Condition if the Driver has been initiated
 */
static int driverInit = 0;
/**
 * Entry index for next terminal
 */
static int termIn = 0;

/**
 * Condition measuring how many writing threads there are
 */
static int writing = 0;

/**
 * Condition dictating that an echo is queued
 */
static int echoWait = 0;

/**
 * Condition dictating that the echo process is yet to be completed
 */
static int echo = 0;


static int snapShot = 0;

static int ssWaiting = 0;

static cond_id_t snapShotSignal;

/**
 * Signal for automatic writing
 */
static cond_id_t writingSig;

/**
 * Signal for echoing to proceed
 */
static cond_id_t echoSig;

/**
 * Buffer for inputted characters
 * Indiscriminate by terminal to ensure keys
 * are echoed in the order they are typed.
 */
static struct buffer echoBuf;

/**
 * Synchronizes with echoBuf to make sure each character is echoed
 * to the correct terminal
 */
static struct buffer echoBufMap;

/**
 * Begins a continual process of echoing until the buffer is empty
 * @param start This routine only waits if it is called from
 * ReceiveInterrupt. If it is called from transmit interrupt,
 * the writing thread is already blocked. Done to avoid
 * deadlock.
 */
void echoToTerminal(int start) {
    if(writing && start) {echoWait = 1; CondWait(echoSig); echoWait = 0;}
    int t = popFromBuffer(&echoBufMap);
    if(snapShot) {ssWaiting += 1; CondWait(snapShotSignal);}
    WriteDataRegister(t,popFromBuffer(&echoBuf));
    terminals[t].tty_out++;

}

/**
 * Called when one inputs to a terminal
 * @param term terminal that received the interrupt
 */
extern void ReceiveInterrupt(int term) {
    Declare_Monitor_Entry_Procedure();
    struct terminal *t = &terminals[term];
    if(t->inputBuf.full) {
        return;
    }
    int empty = t->inputBuf.empty;
    //int empty = t->tempInput.empty;
    if(snapShot) {ssWaiting += 1; CondWait(snapShotSignal);}
    char c = ReadDataRegister(term);
    t->tty_in ++;
    switch(c) {
        case '\r':
            pushToBuffer(&t->inputBuf, '\n');
            pushToBuffer(&echoBuf, '\r');
            pushToBuffer(&echoBufMap, term);
            pushToBuffer(&echoBuf, '\n');
            pushToBuffer(&echoBufMap, term);
            break;
        case '\n':
            pushToBuffer(&t->inputBuf, '\n');
            pushToBuffer(&echoBuf, '\r');
            pushToBuffer(&echoBufMap, term);
            pushToBuffer(&echoBuf, '\n');
            pushToBuffer(&echoBufMap, term);
            break;
        case '\b':
            pullFromBuffer(&t->inputBuf);
            pushToBuffer(&t->inputBuf, '\b');
            pushToBuffer(&echoBuf, '\b');
            pushToBuffer(&echoBufMap, term);
            pushToBuffer(&echoBuf, ' ');
            pushToBuffer(&echoBufMap, term);
            pushToBuffer(&echoBuf, '\b');
            pushToBuffer(&echoBufMap, term);
            break;
        case '\177':
            pullFromBuffer(&t->inputBuf);
            pushToBuffer(&t->inputBuf, '\b');
            pushToBuffer(&echoBuf, '\b');
            pushToBuffer(&echoBufMap, term);
            pushToBuffer(&echoBuf, ' ');
            pushToBuffer(&echoBufMap, term);
            pushToBuffer(&echoBuf, '\b');
            pushToBuffer(&echoBufMap, term);
            break;
        default:
            pushToBuffer(&t->inputBuf, c);
            pushToBuffer(&echoBuf, c);
            pushToBuffer(&echoBufMap, term);
    }
    /**
     * Begin echo process if not already started
     */
    if(!echo) {echo = 1; echoToTerminal(1);}
    /**
     * Alert the ReadTerminal Routine that it can continue
     */
    if(empty && t->reading) {CondSignal(t->readSignal);}


}

extern void TransmitInterrupt(int term) {
    Declare_Monitor_Entry_Procedure();
    struct terminal *t = &terminals[term];
    /**
     * Condition if the echo is waiting for a write operation to finish
     */
    if(echoWait) {
        CondSignal(echoSig);
    }
        /**
         * Case if monitor is in the process of echoing
         */
    else if(echo) {
        /**
         * If the echo buffer has been emptied
         */
        if(echoBuf.empty) {
            echo = 0;
            CondSignal(writingSig);
        }
            /**
             * Continue the echo process
             */
        else {
            echoToTerminal(0);
        }
    }
        /**
         * If no echoing is in process send a message notifying that it is
         * safe to write
         */
    else {
        CondSignal(writingSig);
    }

}

extern int WriteTerminal(int term, char* buf, int buflen) {
    Declare_Monitor_Entry_Procedure();
    if(!terms[term]) {
        return -1;
    }
    if(buflen == 0) return 0;
    struct terminal *t = &terminals[term];
    int c;
    if(writing || echo) {
        CondWait(writingSig);
    }
    writing += 1;
    for(c = 0; c < buflen; c++) {
        char next = buf[c];
        if(snapShot) {ssWaiting += 1; CondWait(snapShotSignal);}
        WriteDataRegister(term,next);
        t->tty_out++;
        CondWait(writingSig);
        if(next == '\n') {
            if(snapShot) {ssWaiting += 1; CondWait(snapShotSignal);}
            WriteDataRegister(term,'\r');
            t->tty_out++;
            CondWait(writingSig);
        }
    }
    CondSignal(echoSig);
    CondSignal(writingSig);
    if(snapShot) {ssWaiting += 1; CondWait(snapShotSignal);}
    t->user_in += c;
    writing -= 1;
    return c;
}

extern int ReadTerminal(int term, char* buf, int buflen) {
    Declare_Monitor_Entry_Procedure();
    if(!terms[term]) {
        return -1;
    }
    if(buflen == 0) return 0;
    struct terminal *t = &terminals[term];
    //struct buffer s;
    if(t->reading) {
        CondWait(t->readThread);
    }
    t->reading = 1;
    int c = 0;
    char next;
    while(!(t->readBuf.empty)) {
        next = popFromBuffer(&t->readBuf);
        //printf("In Read buffer - %c\n",next);
        if(next =='\b') {
            c--;
            if(c < 0) {c = 0;}
            continue;
        }
        if(c < buflen) {
            buf[c] = next;
            c++;
        }
        if(next == '\n') {
            t->reading = 0;
            if(snapShot) {ssWaiting += 1; CondWait(snapShotSignal);}
            t->user_out += c;
            return c;
        }
        if(c >= buflen) {
            break;
        }
    }
    //printf("Past Read Buffer\n");
    while(1) {
        next = popFromBuffer(&t->inputBuf);
        if (next == '\0') {
            CondWait(t->readSignal);
            continue;
        }
        if (next == '\b') {
            c--;
            if (c < 0) { c = 0; }
            if(c >= buflen) {
                pullFromBuffer(&t->readBuf);
            }
            continue;
        }
        if(c < buflen) {
            buf[c] = next;
        } else {
            //printf("Storing in read buffer\n");
            pushToBuffer(&t->readBuf, next);
        }
        c++;
        if(next == '\n') {
            break;
        }
    }
    int i = (c > buflen)? buflen : c;
    //incrementBuffer(&t->inputBuf,i);
    if(snapShot) {ssWaiting += 1; CondWait(snapShotSignal);}
    t->user_out += i;
    t->reading = 0;
    CondSignal(t->readThread);
    return i;
}

extern int InitTerminal(int term) {
    Declare_Monitor_Entry_Procedure();
    if(!driverInit) {
        return -1;
    }
    if(terms[term]) {
        return -1;
    }
    struct terminal t;
    t.term = term;
    t.inputBuf = getBuffer(BUFFER_SIZE);
    t.readBuf = getBuffer(BUFFER_SIZE);
    t.reading = 0;
    t.readSignal = CondCreate();
    t.readThread = CondCreate();
    t.tty_in = 0;
    t.tty_out = 0;
    t.user_in = 0;
    t.user_out = 0;
    terms[term] = 1;
    terminals[term] = t;
    termIn ++;
    return InitHardware(term);
}

extern int TerminalDriverStatistics(struct termstat *stats) {
    Declare_Monitor_Entry_Procedure();
    if(!driverInit) {
        return -1;
    }
    snapShot = 1;
    int tr = 0;
    struct termstat *curTerm;
    while(tr < NUM_TERMINALS) {
        curTerm = &stats[tr];
        if(!terms[tr]) {
            curTerm->tty_in = -1;
            curTerm->tty_out = -1;
            curTerm->user_in = -1;
            curTerm->user_out = -1;
            tr++;
            continue;
        } else {
            curTerm->tty_in = terminals[tr].tty_in;
            curTerm->tty_out = terminals[tr].tty_out;
            curTerm->user_in = terminals[tr].user_in;
            curTerm->user_out = terminals[tr].user_out;
            tr++;
        }
    }
    while(ssWaiting > 0) {
        CondSignal(snapShotSignal);
        ssWaiting -= 1;
    }
    snapShot = 0;
    return 0;
}

extern int InitTerminalDriver() {
    Declare_Monitor_Entry_Procedure();
    /**
     * If the driver has been initiated already, return
     */
    if(driverInit) {
        return -1;
    }

    /**
     * Initialize the allocated space for the terminals to false
     */
    int i;
    for(i = 0; i < NUM_TERMINALS; i++) {
        terms[i] = 0;
    }
    /**
     * Initiate Signals
     */
    writingSig = CondCreate();
    echoSig = CondCreate();
    snapShotSignal = CondCreate();
    /**
     * Initiate Buffers
     */
    echoBuf = getBuffer(BUFFER_SIZE);
    echoBufMap = getBuffer(BUFFER_SIZE);

    /**
     * Set flag that initDriver has finished
     */
    driverInit = 1;
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

static void incrementBuffer(struct buffer *buf, int increments){
    int i = 0;
    while(i < increments) {
        popFromBuffer(buf);
        i++;
    }

}
