#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>
#include <terminals.h>

void writer(void *);

void writer2(void *);

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
    writer(NULL);
    //ThreadCreate(writer, NULL);


    //ThreadCreate(writer2, NULL);
    ThreadWaitAll();
    //writer(NULL);


    exit(0);
}

void
writer(void *arg) {
    int status;

    printf("Doing WriteTerminal... '\n");
    fflush(stdout);
    status = WriteTerminal(1, string, length);
    sleep(100);
    fflush(stdout);
    //a
}
