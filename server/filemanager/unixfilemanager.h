#ifndef UNIXFILEMANAGER
#define UNIXFILEMANAGER

typedef struct{
    unsigned short int occupato;
    unsigned int num_prenotazione;
} Posto;

int create_map(char *fname, int namesize); // crea il file che rappresenta la mappa del cinema se non esiste

int seat_set_flag(int ds, char row, int col, int flag, int nbook); // permette di associare/disassociare un posto da una certa prenotazionre
int seat_get_flag(int ds, char row, int col);

// int get_seats_from_book(int ds, int nbook);


#endif