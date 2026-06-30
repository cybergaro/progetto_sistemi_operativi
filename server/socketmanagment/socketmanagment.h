#ifndef SOCKETMANAGMENT
#define SOCKETMANAGMENT

#include <pthread.h>

extern int *sockets; // array per salvare i socket id dei client
extern int n_clients; // conta il numero dei client attualmente connessi
extern int size_sockets_array; 
extern pthread_mutex_t clients_mutex;

void init_client_list();
void add_client(int socket_id);
void remove_client(int socket_id);

#endif