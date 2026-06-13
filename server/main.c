#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// posix library
#include "filemanager/unixfilemanager.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BACKLOG 10
#define PORT 8080

#define NAME_FILE_MAP "cinema_map.bin"

struct connection_handler_arg {
    int socket_id;
};

struct sockaddr_in server_addr;

typedef struct {
    unsigned short code; // indica il codice della richiesta
    unsigned short row;  // indica la fila del posto a cui si sta facendo riferimento
    unsigned short col;  // indica la colonna del posto
    unsigned int dim;    // rappresenta la dimensione dei dati che seguono il preambolo
} SocketMessagePreamble;

/**
 * Allowed codes:
 * 1) get map with flags
 * 2) send map with flags (il server spedisce al client la mappa con i posti)
 */

void *connection_handler(void *arg) {
    struct connection_handler_arg *data = (struct connection_handler_arg *)arg;
    int client_sock = data->socket_id;

    // creo una nuova sessione sul file
    int fd = open(NAME_FILE_MAP, O_RDWR, 0644);
    if (fd < 0) {
        printf("Error: creation of new session on map file \n");
        close(client_sock);
        return NULL;
    }

    SocketMessagePreamble req;
    ssize_t read_size;

    while (1) {
        read_size = recv(client_sock, &req, sizeof(req), 0);

        if (read_size == 0) {
            printf("Error: client disconnected \n");
            break;
        } else if (read_size < 0) {
            printf("Error: recv \n");
            break;
        } else if (read_size == sizeof(req)) { // dati ricevuti correttamente   
            int req_code = ntohs(req.code);

            printf("request code-> %d \n", req_code);

            switch (req_code){
            case 1: {
                unsigned short int matrix[ROWS][COLS];
                if(get_all_flag(fd, matrix) <0){
                    printf("Error: Get all map flag \n");
                    continue;
                }

                SocketMessagePreamble res;
                res.code = htons(2);
                res.row = 0;
                res.col = 0;
                res.dim = htonl(sizeof(matrix));

                size_t total_size = sizeof(res) + sizeof(matrix);

                unsigned char buffer[total_size];
                memcpy(buffer, &res, sizeof(res));
                memcpy(buffer + sizeof(res), matrix, sizeof(matrix));
                if(send(client_sock, buffer, total_size, 0)<0){
                    printf("Error: send response (2)\n");
                    continue;
                }

                break;
            }
            default:
                break;
            }


        }
    }

    return NULL;
}

void sigpipe_handler(int sig) {
    printf("\n Sig Pipe \n");
}

void sigint_handler(int sig) {
    printf("\n Sig Int \n");
}

int main(int argc, char const *argv[]) {

    printf("Setup... ⏳\n");

    printf("Socket setup \n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int general_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (general_socket == -1) {
        printf("Error: Creation of general socket \n");
        exit(EXIT_FAILURE);
    }

    if (bind(general_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        printf("Error: bind not found \n");
        exit(EXIT_FAILURE);
    }

    if (listen(general_socket, BACKLOG)) {
        printf("Error: listen not found");
        exit(EXIT_FAILURE);
    }

    printf("File setup \n");

    if (create_map(NAME_FILE_MAP) < 0) {
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

    while (1) {
        new_socket = accept(general_socket, NULL, NULL);
        if (new_socket == -1) {
            printf("Error on accept \n");
            continue;
        }

        printf("New connection \n");

        struct connection_handler_arg *arg = malloc(sizeof(struct connection_handler_arg));
        arg->socket_id = new_socket;

        pthread_create(&thread, NULL, connection_handler, arg);
    }

    return 0;
}