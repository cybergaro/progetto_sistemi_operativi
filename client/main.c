#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// librerie posix
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef GUI
#include "gui.h"
#else
#include "cliview.h"
#endif

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8080


typedef struct {
    unsigned short code; // indica il codice della richiesta
    unsigned short row;  // indica la fila del posto a cui si sta facendo riferimento
    unsigned short col;  // indica la colonna del posto
    unsigned int dim;    // rappresenta la dimensione dei dati che seguono il preambolo
} SocketMessagePreamble;

/**
 * Allowed codes:
 * 1) get map with flags
 *
 */

int socket_des;
unsigned short int map[ROWS][COLS]; // matrice che rappresenta lo stato di occupazione dei posti

void new_book() {
    SocketMessagePreamble req;

    req.code = htons(1);
    req.row = 0;
    req.col = 0;
    req.dim = 0;

    if (send(socket_des, &req, sizeof(req), 0) < 0) {
        printf("Error: request of cinema map not found \n");
        return;
    }

    SocketMessagePreamble res;
    recv(socket_des, &res, sizeof(res),0);

    recv(socket_des, map, sizeof(map),0);

    printMap(map);
}

int main(int argc, char const *argv[]) {

    printf("Setup... ⏳\n");

    printf("Socket setup \n");

    socket_des = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_des == -1) {
        printf("Error: Creation of socket \n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    if (connect(socket_des, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        printf("Error: Connection to server \n");
        close(socket_des);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server ✅\n");

    printf("Setup complete ✅\n");

    char buffer[255];
    int op; // codice dell'operazione selezionato dall'utente all'interno del menu

    printf("Options: \n0) 📤 Exit \n1) 🆕 New booking  \n2) 🔧 Manage old booking \n");
    fflush(stdout);

    while (1) {
        fflush(stdout);
        printf("-> ");

        fgets(buffer, sizeof(buffer), stdin);
        sscanf(buffer, "%d", &op); // estraggo il codice dell'operazione

        switch (op) {
        case 0:
            close(socket_des); // chiudo il socket
            printf("Exit success!\n");
            return 0;

        case 1:
            new_book();
            break;

        case 2:
            printReservations();
            break;

        default:
            printf("⚠️  Codice operazione non valido\n");
            fflush(stdout);
            break;
        }

        op = -1;
    };
}