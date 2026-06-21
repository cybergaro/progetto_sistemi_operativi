#include "socketmanagment.h"
#include <stdlib.h>

#include <pthread.h>

pthread_mutex_t lock;

void init_client_list() {
    size_sockets_array = 10; 
    n_clients = 0;
    sockets = malloc(size_sockets_array * sizeof(int));
    pthread_mutex_init(&lock, NULL);
}

void add_client(int socket_id) {
    pthread_mutex_lock(&lock);

    if (n_clients >= size_sockets_array) {
        size_sockets_array *= 2;
        sockets = realloc(sockets, size_sockets_array * sizeof(int));
    }

    sockets[n_clients] = socket_id;
    n_clients++;

    pthread_mutex_unlock(&lock); 
}

void remove_client(int socket_id) {
    pthread_mutex_lock(&lock);

    for (int i = 0; i < n_clients; i++) {
        if (sockets[i] == socket_id) {
            for (int j = i; j < n_clients - 1; j++) {
                sockets[j] = sockets[j + 1];
            }
            n_clients--;
            break;
        }
    }

    pthread_mutex_unlock(&lock);
}