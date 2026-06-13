#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// posix library
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "unixfilemanager.h"

#define BACKLOG 10
#define PORT 8080


struct connection_handler_arg{
    int socket_id;
};

struct sockaddr_in server_addr;

void *connection_handler(void *arg){
    struct connection_handler_arg *data = (struct connection_handler_arg *)arg;

    printf("ok \n");


    return NULL;
}

int main(int argc, char const *argv[]) {

    printf("Setup... ⏳\n");
    
    printf("Socket setup \n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int general_socket = socket(PF_INET, SOCK_STREAM, 0);
    if(general_socket == -1){
        printf("Error: Creation of general socket \n");
        exit(EXIT_FAILURE);
    }

    if(bind(general_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        printf("Error: bind not found \n");
        exit(EXIT_FAILURE);
    }

    if(listen(general_socket, BACKLOG)){
        printf("Error: listen not found");
        exit(EXIT_FAILURE);
    }

    printf("Setup complete ✅\n");
    printf("Waiting for connection \n");

    int new_socket;
    pthread_t thread;

    while (1){
        new_socket = accept(general_socket, NULL, NULL);
        if(new_socket == -1){
            printf("Error on accept \n");
            continue;
        }

        printf("New connection \n");

        struct connection_handler_arg *arg = malloc(sizeof(struct connection_handler_arg));
        arg->socket_id = new_socket;

        pthread_create(&thread, NULL, connection_handler, NULL);
    }
    

    return 0;
}