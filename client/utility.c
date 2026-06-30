#include "utility.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>

#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\x1b[1;38;5;208m"

void printMenu() {
    printf("Options: \n0) 📤 Exit \n1) 🆕 New booking  \n2) 🔧 Manage old booking \n");
    fflush(stdout);
}

// per evitare di creare problemi inseendo il booknumber come variabile globale creo una variabile 
// "cache" che utilizza solo print map quando non gli viene passato un boocknumber e quindi ristampa il precedente
unsigned int cache_booknumber; 

void printMap(unsigned short int map[ROWS][COLS], unsigned int booknumber) {
    if(booknumber != 0)
        cache_booknumber = booknumber;
    else
        booknumber = cache_booknumber;

    char buffer[4096]; 
    buffer[0] = '\0';

    char *ptr = buffer;

    ptr += sprintf(ptr, "\nBooknumber: \033[1;33m%u\033[0m\n", booknumber);
    ptr += sprintf(ptr, "\n========================================= SCREEN =========================================\n\n");

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            ptr += sprintf(ptr, "+-----");
        }
        ptr += sprintf(ptr, "+\n");

        for (int j = 0; j < COLS; j++) {
            ptr += sprintf(ptr, "| %s%c%2d%s ", map[i][j] == 0 ? COLOR_GREEN : map[i][j] == 1 ? COLOR_YELLOW
                                                                                              : COLOR_RED,
                           'A' + i, j + 1, COLOR_RESET);
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

/*
return = 0  ==> ho letto un posto valido
return = -1 ==> posto non valido
return = 1  ==> l'utente vuole terminare salvando
return = 2 ==> l'utente vuole terminare non salvando
*/
int getSeatNumber(int *numero, char *lettera) {
    char input[255];

    printf("Seat identificator (es. A12, A 2, C 15) \nWrite '0' to exit/cancell or '1' to confirm-> ");
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Error: fgets\n");
        fflush(stdout);
        return -1;
    }

    input[strcspn(input, "\n")] = '\0';

    if (strcmp(input, "1") == 0) // esco salvando
        return 1;

    if (strcmp(input, "0") == 0) // esco e non salvo
        return 2;

    char extra[2];
    int elementi_letti = sscanf(input, " %c %d%1s", lettera, numero, extra);

    if (elementi_letti != 2 || !isupper((char)*lettera)) {
        printf("Error: invalid format\n");
        fflush(stdout);
        return -1;
    }

    // controllo che l'elemento è effettivamente presente nella matrice
    if ((*lettera) - 'A' < 0 || (*lettera) - 'A' >= ROWS) {
        printf("Error: Letter out of range \n");
        fflush(stdout);
        return -1;
    }
    if ((*numero) - 1 < 0 || (*numero) - 1 >= COLS) {
        printf("Error: Number out of range\n");
        fflush(stdout);
        return -1;
    }

    return 0;
}

void printHistory() {
    system("clear");

    lseek(history_des, 0, SEEK_SET);

    HistoryRecord record;

    printf("\n+------------+------------------+--------+ \n|    Code    |       Date       | N.seat | \n");

    while (read(history_des, &record, sizeof(record)) != 0) {

        struct tm *tm_info = localtime(&record.time);
        char buffer[26];
        strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M", tm_info);

        printf("+------------+------------------+--------+ \n| %10d | %s |  %4d  | \n", record.booknumber, buffer, record.nseats);
    }

    printf("+------------+------------------+--------+\n\n");
}

void saveToHistory(HistoryRecord *record) {
    lseek(history_des, 0, SEEK_END);
    write(history_des, record, sizeof(HistoryRecord));
}

void removeFromHistory(unsigned int booknumber) {
    lseek(history_des, 0, SEEK_SET);

    HistoryRecord record;
    int found = 0;
    off_t read_pos, write_pos;

    while (read(history_des, &record, sizeof(HistoryRecord)) == sizeof(HistoryRecord)) {
        if (record.booknumber == booknumber) {
            found = 1;
            write_pos = lseek(history_des, 0, SEEK_CUR) - sizeof(HistoryRecord);
            read_pos = lseek(history_des, 0, SEEK_CUR);
            break;
        }
    }

    if (!found) {
        return;
    }

    while (1) {
        lseek(history_des, read_pos, SEEK_SET);
        if (read(history_des, &record, sizeof(HistoryRecord)) < sizeof(HistoryRecord)) {
            break;
        }

        lseek(history_des, write_pos, SEEK_SET);
        if (write(history_des, &record, sizeof(HistoryRecord)) < 0) {
            printf("Error: fallita la scrittura durante la rimozione dello storico\n");
            return;
        }

        read_pos += sizeof(HistoryRecord);
        write_pos += sizeof(HistoryRecord);
    }

    if (ftruncate(history_des, write_pos) < 0) {
        printf("Error:truncate histoy\n");
    } 
}