#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>
#include <terminals.h>

void reader(void *);

//char string[] = "abcdefghijklmnopqrstuvwxyz\r\n:^)";
char string[] = "..........................................\r\n..........................................\r\n..........................................\r\n..........................................\r\n..........................................\r\n..........................................\r\n..........................................\r\n";
int length = sizeof(string) - 1;

int main(int argc, char **argv)
{
    InitTerminalDriver();
    InitTerminal(1);
    //InitTerminal(2);
    if (argc > 1) HardwareOutputSpeed(1, atoi(argv[1]));
    if (argc > 2) HardwareInputSpeed(1, atoi(argv[2]));
    ThreadCreate(reader, NULL);
    reader(NULL);



    //ThreadCreate(writer2, NULL);
    ThreadWaitAll();
    //writer(NULL);


    exit(0);
}

void
reader(void *arg) {
    int status;
    int status2;
    char *x = malloc(16*sizeof(char));
    char *y = malloc(16*sizeof(char));
    printf("Doing ReadTerminal... '\n");
    fflush(stdout);
    status = ReadTerminal(1, x, 16);
    printf("X: %.*s %d\n",  16*sizeof(char),x, status);
    status = ReadTerminal(1, y, 16);
    printf("Y: %.*s %d\n",  16*sizeof(char),y, status);

}