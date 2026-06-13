#ifndef UNIXFILEMANAGER
#define UNIXFILEMANAGER
#define ROWS 10
#define COLS 15


typedef struct{
    unsigned short int flag;
    unsigned int nbook;
} Seat;

int create_map(char *fname); // crea il file che rappresenta la mappa del cinema se non esiste

int seat_set_flag(int ds, char row, int col, int flag, int nbook); // permette di associare/disassociare un posto da una certa prenotazionre
int seat_get_flag(int ds, char row, int col);
int get_all_flag(int ds);

// int get_seats_from_book(int ds, int nbook);


#endif