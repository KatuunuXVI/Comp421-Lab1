#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <terminals.h>


int
main(int argc, char **argv)
{
    InitTerminalDriver();
    InitTerminal(1);
    InitTerminal(2);
    if (argc > 1) HardwareOutputSpeed(1, atoi(argv[1]));
    if (argc > 2) HardwareInputSpeed(1, atoi(argv[2]));
    char x[8];
    while(1);
    exit(0);
}