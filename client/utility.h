#ifndef CLI_VIEW
#define CLI_VIEW

#include <sys/types.h>

#define ROWS 10
#define COLS 15

#define HISTORY_NAME "history.bin"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8080

typedef struct {
    unsigned short code;     // indica il codice della richiesta
    unsigned short row;      // indica la fila del posto a cui si sta facendo riferimento
    unsigned short col;      // indica la colonna del posto
    unsigned int booknumber; // indica il codice della prenotazione
    unsigned int dim;        // rappresenta la dimensione dei dati che seguono il preambolo
    int short_data;          // questo campo non ha un uso specifico ma può essere utilizzato per inviare delle informazioni all'occorrenza
} SocketMessagePreamble;

/**
 * Allowed codes:
 * 1)  get map with flags
 * 2)  send map with flags (il server spedisce al client la mappa con i posti)
 * 3)  request seat (imposta il flag di un posto a 1 indicando il fatto che su quel posto è in corso una prenotazione)
 * 4)  reserved seat (il posto ora ha il flag di prenotazine pending f=1)
 * 5)  seat not bookable (questo posto non risulta prenotabile)
 * 6)  confirm book (conferma la prenotazione)
 * 7)  cancell book (rimuove tutti i posti di una prenotazione)
 * 8)  send booknumber (usato dal server per comunicare al client il codice di prenotazione)
 * 9)  remove single seat during the booking operation
 * 10) broadcast single seat update (comunico al client che un certo posto ha fatto un cambiamento di flag)
 */

 
// descrittore del file dello storico di prenotazioni dichiarato in utilty.c
#if defined(_WIN32) || defined(WINDOWS)
    #include <windows.h>
    extern HANDLE history_des; 
#else
    extern int history_des;
#endif

typedef struct{
    unsigned int booknumber;
    unsigned int nseats;
    time_t time;
} HistoryRecord;

void printMenu();
void printMap(unsigned short int map[ROWS][COLS], unsigned int booknumber);
int getSeatNumber(int *numero, char *lettera);
void printHistory(void);
void saveToHistory(HistoryRecord *record);
void removeFromHistory(unsigned int booknumber);

#endif