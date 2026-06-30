#ifndef UNIXFILEMANAGER
#define UNIXFILEMANAGER

#include <pthread.h>

#define ROWS 10
#define COLS 15

extern pthread_mutex_t seat_mutexes[ROWS * COLS];  // mutex per la gestione del file

typedef struct {
    unsigned short int flag;
    unsigned int nbook;
} Seat;

int create_map(char *fname); // crea il file che rappresenta la mappa del cinema se non esiste

int seat_set_flag(int ds, int row, int col, int flag, int nbook); // permette di associare/disassociare un posto da una certa prenotazionre
int seat_get_flag(int ds, int row, int col);
int get_all_flag(int ds, unsigned short int matrix[ROWS][COLS], short int mask, int nbook);
int set_all_flag_from_nbook(int ds, int flag, int nbook, int **modified_seats, int *num_modified); // permete di impostare tutti i flag associati ad una certa prenotazione ad un determinato booknumber

#endif