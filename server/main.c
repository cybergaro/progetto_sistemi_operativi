#include "socketmanagment/socketmanagment.h"
#include "utility/utility.h"
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
#define PORT 8081

#define NAME_FILE_MAP "cinema_map.bin"

struct connection_handler_arg {
    int socket_id;
};

struct sockaddr_in server_addr;

typedef struct {
    unsigned short code;     // indica il codice della richiesta
    unsigned short row;      // indica la fila del posto a cui si sta facendo riferimento
    unsigned short col;      // indica la colonna del posto
    unsigned int booknumber; // indica il codice della prenotazione
    unsigned int dim;        // rappresenta la dimensione dei dati che seguono il preambolo
    int short_data;          // questo campo non ha un uso specifico ma può essere utilizzato per inviare delle informazioni all'occorrenza
} SocketMessagePreamble;

/**
 * Allowed codes:
 * 1)  get map with flags
 * 2)  send map with flags (il server spedisce al client la mappa con i posti)
 * 3)  request seat (imposta il flag di un posto a 1 indicando il fatto che su quel posto è in corso una prenotazione)
 * 4)  reserved seat (il posto ora ha il flag di prenotazine pending f=1)
 * 5)  seat not bookable (questo posto non risulta prenotabile)
 * 6)  confirm book (conferma la prenotazione)
 * 7)  cancell book (rimuove tutti i posti di una prenotazione)
 * 8)  send booknumber (usato dal server per comunicare al client il codice di prenotazione)
 * 9)  remove single seat during the booking operation
 * 10) broadcast single seat update (comunico al client che un certo posto ha fatto un cambiamento di flag)
 */

int general_socket; // descrittore del socket

void send_brodcast_message(void *message, int message_size, int client_sock) {
    if (sockets == NULL || n_clients == 0) { // controllo se ci sono dei client connessi
        return;
    }

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < n_clients; i++) {
        if (client_sock == sockets[i])
            continue;

        printf("Send brodcast message to %d\n", sockets[i]);

        if (send(sockets[i], message, message_size, 0) < 0) {
            printf("Error: fallito invio broadcast al socket %d\n", sockets[i]);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void create_multiple_delete_socket_messages(int *modified_seats, int num_modified, int client_sock) { // uso questa funzione per creare dei messaggi broadcast multipli per segnalare che si sono liberati più posti
    SocketMessagePreamble res;
    res.code = htons(10);
    res.short_data = 0; // indico che il posto si è liberato
    res.booknumber = 0;
    res.dim = 0;
    
    int row;
    int col;

    for (int x = 0; x < num_modified; x++) {
        col = modified_seats[x]%COLS;
        row = (modified_seats[x]-col)/COLS;
        
        res.row = htons(row);
        res.col = htons(col);

        send_brodcast_message(&res, sizeof(res), client_sock);
    }
}

void *connection_handler(void *arg) {
    // creo una maschera per le segnalazioni
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    struct connection_handler_arg *data = (struct connection_handler_arg *)arg;
    int client_sock = data->socket_id;

    free(data);

    // creo una nuova sessione sul file
    int fd = open(NAME_FILE_MAP, O_RDWR, 0644);
    if (fd < 0) {
        printf("Error: creation of new session on map file \n");
        close(client_sock);
        return NULL;
    }

    // associo al client un codice di prenotazione
    unsigned int booknumber;

    SocketMessagePreamble req;
    ssize_t read_size;

    while (1) {
        read_size = recv(client_sock, &req, sizeof(req), MSG_WAITALL);

        if (read_size == 0) {
            printf("Client disconnected! \n");
            fflush(stdout);

            // rilascio tutti i posti che quel cliente stava prenotando
            int *modified_seats = NULL;
            int change_counter;

            set_all_flag_from_nbook(fd, 3, booknumber, &modified_seats, &change_counter);

            create_multiple_delete_socket_messages(modified_seats, change_counter, client_sock);

            free(modified_seats);

            // rimuovo il socket dalla lista
            remove_client(client_sock);

            close(fd);
            return NULL;
        } else if (read_size < 0) {
            printf("Error: recv \n");
            break;
        } else if (read_size == sizeof(req)) { // dati ricevuti correttamente
            int req_code = ntohs(req.code);

            if (ntohl(req.booknumber) != 0) { // se nella richiesta c'è un codice di prenotazione uso quello
                booknumber = ntohl(req.booknumber);
            } else { // se nella richiesta non c'è un codice di prenotazione ne genero uno nuovo
                booknumber = new_book_number();
            }

            printf("request code-> %d \n", req_code);
            fflush(stdout);

            switch (req_code) {
            case 1: { // richiesta della mappa dei posti
                unsigned short int matrix[ROWS][COLS];
                if (get_all_flag(fd, matrix, 1, booknumber) < 0) {
                    printf("Error: Get all map flag \n");
                    continue;
                }

                SocketMessagePreamble res;
                res.code = htons(2);
                res.row = 0;
                res.col = 0;
                res.booknumber = htonl(booknumber);
                res.dim = htonl(sizeof(matrix));
                res.short_data = 0;

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

                pthread_mutex_lock(&seat_mutexes[row * COLS + col]);

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
                res.short_data = 0;

                if (flag == 0) {
                    // salvo nel file che quel posto è in fase pending
                    if (seat_set_flag(fd, row, col, 1, booknumber) < 0) {
                        printf("Error: seat set flag f = 1");
                    }

                    res.code = htons(4);
                
                    printf("Seat %c%d is pendign for %d\n", row + 'A', col + 1, booknumber);
                
                } else {
                    res.code = htons(5);
                }

                pthread_mutex_unlock(&seat_mutexes[row * COLS + col]);

                if (send(client_sock, &res, sizeof(res), 0) < 0) {
                    printf("Error: send response (4/5)\n");
                    continue;
                }


                // allerto tutti gli altri client della scelta di questo client
                if(flag == 0){
                    res.code = htons(10);
                    res.short_data = htonl(2); // indico che il posto si è occupato
                    send_brodcast_message(&res, sizeof(res), client_sock);
                }

                break;
            }
            case 6: {
                // confermo la prenotazione
                set_all_flag_from_nbook(fd, 2, booknumber, NULL, NULL);

                printf("confermo la prenotazione %d\n", booknumber);

                SocketMessagePreamble res;
                res.code = htons(8);
                res.row = 0;
                res.col = 0;
                res.short_data = 0;
                res.booknumber = booknumber;
                res.dim = 0;

                if (send(client_sock, &res, sizeof(res), 0) < 0) {
                    printf("Error: send response (8) \n");
                    continue;
                }

                break;
            }
            case 7: {
                // cancello una prenotazione
                printf("Sto pulendo la cache della prenotazione %d\n", booknumber);
                fflush(stdout);

                int *modified_seats = NULL;
                int change_counter;
                set_all_flag_from_nbook(fd, 0, booknumber, &modified_seats, &change_counter);

                create_multiple_delete_socket_messages(modified_seats, change_counter, client_sock);

                free(modified_seats);

                unsigned short int matrix[ROWS][COLS];
                if (get_all_flag(fd, matrix, 0, 0) < 0) {
                    printf("Error: Get all map flag \n");
                    continue;
                }

                booknumber = 0;

                break;
            }

            case 9: {
                // elimino un posto da una prenotazione
                int row = ntohs(req.row);
                int col = ntohs(req.col);

                pthread_mutex_lock(&seat_mutexes[row * COLS + col]);

                if (seat_set_flag(fd, row, col, 0, booknumber) < 0) {
                    printf("Error: seat set flag f = 0");
                }

                pthread_mutex_unlock(&seat_mutexes[row * COLS + col]);

                // allerto tutti gli altri client della scelta di questo client
                SocketMessagePreamble res;
                res.code = htons(10);
                res.short_data = htonl(0); // indico che il posto si è liberato
                res.row = req.row;
                res.col = req.col;
                res.booknumber = 0;
                res.dim = 0;

                send_brodcast_message(&res, sizeof(res), client_sock);

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

    close(general_socket); // faccio in modo che nessuno si possa più connettere

    // disconnetto tutti i client
    for (int i = 0; i < n_clients; i++){
        if (sockets[i] > 0) {
            printf("chiusura socket %d\n", sockets[i]);
            close(sockets[i]);
        }
    }

    
    printf("Connections with all clients are closed\n");
    fflush(stdout);
    
    // creo una nuova sessione sul file
    int fd = open(NAME_FILE_MAP, O_RDWR, 0644);
    if (fd < 0) {
        printf("Error: creation of new session on map file \n");
        exit(EXIT_FAILURE);
    }

    // rilascio tutti i posti che qualcuno stava prenotando
    int *modified_seats = NULL;
    int change_counter;

    set_all_flag_from_nbook(fd, 3, 0, &modified_seats, &change_counter);
    
    exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[]) {

    printf("Setup... ⏳\n");

    printf("Socket setup \n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    general_socket = socket(PF_INET, SOCK_STREAM, 0);
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

    init_client_list(); // inizializzo un area di memoria dinamica dedicata al salvataggio dei socket id

    printf("File setup \n");

    if (create_map(NAME_FILE_MAP) < 0) {
        printf("Error: file map creation \n");
        exit(EXIT_FAILURE);
    }

    printf("Mutex setup \n");

    for (int i = 0; i < ROWS * COLS; i++) {
        if (pthread_mutex_init(&seat_mutexes[i], NULL) != 0) {
            printf("Error: initialization of mutex %d \n", i);
            exit(EXIT_FAILURE);
        }
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

        add_client(new_socket); // aggiungo il socket che si è creato alla lista

        struct connection_handler_arg *arg = malloc(sizeof(struct connection_handler_arg));
        arg->socket_id = new_socket;

        pthread_create(&thread, NULL, connection_handler, arg);
    }

    return 0;
}