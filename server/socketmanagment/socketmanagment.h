#ifndef SOCKETMANAGMENT
#define SOCKETMANAGMENT


int *sockets; // array per salvare i socket id dei client
int n_clients; // conta il numero dei client attualmente connessi
int size_sockets_array; 

void init_client_list();
void add_client(int socket_id);
void remove_client(int socket_id);

#endif