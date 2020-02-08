#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>
#include <terminals.h>

/**
 * Struct fora Character Buffer
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

int main(int argc, char **argv)
{
    struct buffer b = getBuffer(8);
    int c;
    for(c = 0; c < 9; c++) {
        pushToBuffer(&b,'P');
    }
    int d;
    for(d = 0; d < 3; d++) {
        popFromBuffer(&b);
    }
    int e;
    for(e = 0; e < 3; e++) {
        pushToBuffer(&b,'D');
    }
    printf("%.*s\n", sizeof(char)*8,b.b);
    char x = popFromBuffer(&b);
    printf("%c\n",x);
    printf("%.*s\n", sizeof(char)*8,b.b);
    exit(0);
}

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