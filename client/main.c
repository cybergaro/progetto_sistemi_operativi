#include <stdio.h>
#include <stdlib.h>

#ifdef GUI
#include "gui.h"
#else
#include "cliview.h"
#endif

int main(int argc, char const *argv[]) {
    char buffer[255];
    int op; // codice dell'operazione selezionato dall'utente all'interno del menu

    printf("Le tue opzioni: \n0) 📤 Esci \n1) 🆕 Nuova prenotazione \n2) ❌ Gestisci una vecchia prenotazione \n");
    fflush(stdout);

    while (1) {
        fflush(stdout);
        printf("-> ");
        
        fgets(buffer, sizeof(buffer), stdin);
        sscanf(buffer, "%d", &op); // estraggo il codice dell'operazione

        switch (op) {
        case 0:
            printf("Sei uscito correttamente!\n");
            fflush(stdout);
            return 0;

        case 1:
            break;

        case 2:
            printReservations();
            break;

        default:
            printf("⚠️  Codice operazione non valido\n");
            fflush(stdout);
            break;
        }

        op = -1;
    };
}