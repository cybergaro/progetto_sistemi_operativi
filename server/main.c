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
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BACKLOG 10
#define PORT 8080

#define NAME_FILE_MAP "cinema_map.bin"

struct connection_handler_arg {
    int socket_id;
};

struct sockaddr_in server_addr;
int sems; // semafori

typedef struct {
    unsigned short code;     // indica il codice della richiesta
    unsigned short row;      // indica la fila del posto a cui si sta facendo riferimento
    unsigned short col;      // indica la colonna del posto
    unsigned int booknumber; // indica il codice della prenotazione
    unsigned int dim;        // rappresenta la dimensione dei dati che seguono il preambolo
} SocketMessagePreamble;

/**
 * Allowed codes:
 * 1) get map with flags
 * 2) send map with flags (il server spedisce al client la mappa con i posti)
 * 3) request seat (imposta il flag di un posto a 1 indicando il fatto che su quel posto è in corso una prenotazione)
 * 4) reserved seat (il posto ora ha il flag di prenotazine pending f=1)
 * 5) seat not bookable (questo posto non risulta prenotabile)
 * 6) confirm book (conferma la prenotazione)
 */

int new_book_number() {
    return (int)time(NULL);
}

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

    // associo al client un codice di prenotazione
    int booknumber = new_book_number();

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

            if (req.booknumber != 0) { // aggiorno il codice di prenotazione
                booknumber = ntohl(req.booknumber);
            }

            printf("request code-> %d \n", req_code);

            switch (req_code) {
            case 1: { // richiesta della mappa dei posti
                unsigned short int matrix[ROWS][COLS];
                if (get_all_flag(fd, matrix, 1) < 0) {
                    printf("Error: Get all map flag \n");
                    continue;
                }

                SocketMessagePreamble res;
                res.code = htons(2);
                res.row = 0;
                res.col = 0;
                res.booknumber = htonl(booknumber);
                res.dim = htonl(sizeof(matrix));

                size_t total_size = sizeof(res) + sizeof(matrix);

                unsigned char buffer[total_size];
                memcpy(buffer, &res, sizeof(res));
                memcpy(buffer + sizeof(res), matrix, sizeof(matrix));
                if (send(client_sock, buffer, total_size, 0) < 0) {
                    printf("Error: send response (2)\n");
                    continue;
                }

                break;
            }
            case 3: { // richiesta dell'assegnazione di un posto
                int row = ntohs(req.row);
                int col = ntohs(req.col);

                int flag = seat_get_flag(fd, row, col);
                if (flag < 0) {
                    printf("Error: seat get flag \n");
                    continue;
                }

                SocketMessagePreamble res;
                res.row = htons(row);
                res.col = htons(col);
                res.booknumber = 0;
                res.dim = 0;

                if (flag == 0) {
                    // salvo nel file che quel posto è in fase pending
                    if (seat_set_flag(fd, row, col, 1, booknumber) < 0) {
                        printf("Error: seat set flag f = 1");
                    }

                    res.code = htons(4);
                } else {
                    res.code = htons(5);
                }

                if (send(client_sock, &res, sizeof(res), 0) < 0) {
                    printf("Error: send response (4/5)\n");
                    continue;
                }

                printf("Seat %c%d is pendign for %d\n", row + 'A', col, booknumber);

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

    printf("Semaphore setup \n");

    sems = semget(IPC_PRIVATE, ROWS * COLS, IPC_CREAT | 0666);

    if (sems < 1) {
        printf("Error: creation of semaphore \n");
        exit(EXIT_FAILURE);
    }

    unsigned short valori[ROWS * COLS];
    for (int i = 0; i < ROWS * COLS; i++) valori[i] = 1;

    union semun argomento;
    argomento.array = valori;

    if (semctl(sems, 0, SETALL, argomento) == -1) {
        perror("Error: initialization of semaphores");
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