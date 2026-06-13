#include "cliview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_RED "\033[31m"

#define ROWS 10
#define COLS 15

void printMap(unsigned short int map[ROWS][COLS]) {
    system("clear");

    char buffer[4096];
    buffer[0] = '\0';

    char *ptr = buffer;

    ptr += sprintf(ptr, "\n\n========================================= SCHERMO =========================================\n\n");

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            ptr += sprintf(ptr, "+-----");
        }
        ptr += sprintf(ptr, "+\n");

        for (int j = 0; j < COLS; j++) {
            ptr += sprintf(ptr, "| %s%c%2d%s ", map[i][j] == 0 ? COLOR_GREEN : COLOR_RED, 'A' + i, j + 1, COLOR_RESET);
        }
        ptr += sprintf(ptr, "|\n");
    }

    for (int j = 0; j < COLS; j++) {
        ptr += sprintf(ptr, "+-----");
    }
    ptr += sprintf(ptr, "+\n\n\n");

    fputs(buffer, stdout);

    fflush(stdout);
}

void printReservations(){
    
    printf("\n+------+----------+--------+ \n| Code |   Date   | N.seat | \n");

    for (int i = 0; i < 6; i++)
    {
        printf("+------+----------+--------+ \n| %4d | 12/10/25 |  %4d  | \n", i, i);
    }

    printf("+------+----------+--------+\n\n");
    
}