#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

// posix library
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "filemanager/unixfilemanager.h"

#define BACKLOG 10
#define PORT 8080

#define NAME_FILE_MAP "cinema_map.bin"

struct connection_handler_arg{
    int socket_id;
};

struct sockaddr_in server_addr;

void *connection_handler(void *arg){
    struct connection_handler_arg *data = (struct connection_handler_arg *)arg;

    // creo una nuova sessione sul file
    int fd = open(NAME_FILE_MAP, O_RDWR, 0644);
    if(fd<0){
        printf("Error: creation of new session on map file \n");
        return -1;
    }

    return NULL;
}

void sigpipe_handler(int sig){
    printf("\n Sig Pipe \n");
}

void sigint_handler(int sig){
    printf("\n Sig Int \n");
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

    printf("File setup \n");
    
    if(create_map(NAME_FILE_MAP) < 0){
        printf("Error: file map creation \n");
        exit(EXIT_FAILURE);
    }

    printf("Defining the management of signals\n");

    struct sigaction sa_pipe;
    sigemptyset(&sa_pipe.sa_mask);
    sa_pipe.sa_flags = SA_RESTART;
    sa_pipe.sa_handler = &sigpipe_handler;

    sigaction(SIGPIPE, &sa_pipe, NULL);

    struct sigaction sa_int;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sa_int.sa_handler = &sigint_handler;

    sigaction(SIGINT, &sa_int, NULL);


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