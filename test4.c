#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <terminals.h>

void writer1(void *);
void writer2(void *);
void writer3(void *);
void reader(void *);
void reader2(void *);

char string1[] = "abcdefghijklmnopqrstuvwxyz";
//char string1[] = "------------------------------\n";
int length1 = sizeof(string1) - 1;

char string2[] = "0123456789";
char string3[] = "|||||||||||||||||||||||||||||";
int length2 = sizeof(string2) - 1;
int length3 = sizeof(string3) - 1;

int
main(int argc, char **argv)
{
    InitTerminalDriver();
    InitTerminal(1);
    InitTerminal(2);
    //InitTerminal(3);
    if (argc > 1) HardwareOutputSpeed(1, atoi(argv[1]));
    if (argc > 2) HardwareInputSpeed(1, atoi(argv[2]));
    ThreadCreate(reader, NULL);
    ThreadCreate(writer1, NULL);
    ThreadCreate(writer2, NULL);
    ThreadCreate(writer3, NULL);
    ThreadCreate(writer1, NULL);
    ThreadCreate(writer2, NULL);

    ThreadCreate(reader, NULL);
    ThreadCreate(reader, NULL);
    ThreadCreate(reader, NULL);
    ThreadCreate(reader, NULL);
    ThreadCreate(writer3, NULL);
    ThreadCreate(writer1, NULL);
    ThreadCreate(writer2, NULL);
    ThreadCreate(writer3, NULL);
    ThreadCreate(reader2, NULL);
    ThreadWaitAll();

    exit(0);
}

void
writer1(void *arg)
{
    int status;
    while(1) {
        status = WriteTerminal(1, "Bitch ass, Bitch Bitch Ass Nigga", length1);
    }

    if (status != length1)
	fprintf(stderr, "Error: writer1 status = %d, length1 = %d\n",
	    status, length1);
}

void
writer2(void *arg)
{
    int status;
    while(1) {
        WriteTerminal(2, "Doodily Ding Dong Tik Tok", sizeof("Doodily Ding Dong Tik Tok"));
    }

    if (status != length2)
	fprintf(stderr, "Error: writer2 status = %d, length2 = %d\n",
	    status, length2);
}


void
writer3(void *arg)
{
    int status;
    while(1) {
        WriteTerminal(2, "Cuz we are the Aqua Teens", length3);
    }

    if (status != length3)
        fprintf(stderr, "Error: writer2 status = %d, length2 = %d\n",
                status, length2);
}

void
reader(void *arg)
{
    int status;
    while(1) {
        printf("Reader\n");
        char x[8];
        char y[8];
        char z[8];
        status = ReadTerminal(1, x, 8);
        printf("X: %.*s -> %d\n", sizeof(char) * status, x, status);
        status = ReadTerminal(1, y, 8);
        printf("Y: %.*s -> %d\n", sizeof(char) * status, y, status);
        status = ReadTerminal(1, z, 8);
        printf("Z: %.*s -> %d\n", sizeof(char) * status, z, status);
    }
    /*char y[8];
    status = ReadTerminal(1, y, 8);
    printf("Y: %.*s -> %d\n", sizeof(char) * 8, y, status);
    char z[8];
    status = ReadTerminal(1, z, 8);
    printf("Z: %.*s -> %d\n", sizeof(char) * 8, z, status);*/
    //}
}

void
reader2(void *arg)
{
    int status;
    char y[8];
    while(1) {
        status = ReadTerminal(2, y, 8);
        printf("Y: %.*s -> %d\n", sizeof(char) * 8, y, status);
    }
}