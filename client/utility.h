#ifndef CLI_VIEW
#define CLI_VIEW

#include <sys/types.h>

#define ROWS 10
#define COLS 15

#define HISTORY_NAME "history.bin"

int history_des; // descrittore del file history

typedef struct{
    unsigned int booknumber;
    unsigned int nseats;
    time_t time;
} HistoryRecord;

void printMenu();
void printMap(unsigned short int map[ROWS][COLS]);
int getSeatNumber(int *numero, char *lettera);
void printHistory(void);
void saveToHistory(HistoryRecord *record);

#endif