#include "cliview.h"
#include <stdio.h>
#include <string.h>

#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_RED "\033[31m"

void printMap()
{
    int righe = 6;
    int colonne = 10;
    
    char buffer[4096]; 
    buffer[0] = '\0';

    char *ptr = buffer;

    // Aggiungiamo l'intestazione
    ptr += sprintf(ptr, "\n\n========================== SCHERMO ==========================\n\n");

    for (int i = 0; i < righe; i++){
        for (int j = 0; j < colonne; j++){
            ptr += sprintf(ptr, "+-----");
        }
        ptr += sprintf(ptr, "+\n");

        for (int j = 0; j < colonne; j++){

            ptr += sprintf(ptr, "| %s%c%2d%s ", COLOR_GREEN, 'A' + i, j+1, COLOR_RESET);
        }
        ptr += sprintf(ptr, "|\n");
    }

    // Ultima riga di chiusura della tabella
    for (int j = 0; j < colonne; j++){
        ptr += sprintf(ptr, "+-----");
    }
    ptr += sprintf(ptr, "+\n\n\n");

    fputs(buffer, stdout);
    
    fflush(stdout); 
}
