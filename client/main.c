#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// librerie posix
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#ifdef GUI
#include "gui.h"
#else
#include "utility.h"
#endif

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8080

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
 * 7) cancell book (rimuove tutti i posti di una prenotazione)
 * 8) send booknumber (usato dal server per comunicare al client il codice di prenotazione)
 */

int socket_des;
unsigned short int map[ROWS][COLS]; // matrice che rappresenta lo stato di occupazione dei posti

unsigned int get_map(unsigned int booknumber) { // il valore di ritorno è il booknumber che viene assegnato dal server
    SocketMessagePreamble req, res;

    req.code = htons(1);
    req.row = 0;
    req.col = 0;
    req.dim = 0;
    req.booknumber = booknumber != 0 ? htonl(booknumber) : 0;

    if (send(socket_des, &req, sizeof(req), 0) < 0) {
        printf("Error: request of cinema map not found \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (recv(socket_des, &res, sizeof(res), 0) < 0) {
        printf("Error during recv preamble \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (ntohs(res.code) != 2 || ntohl(res.dim) != sizeof(map)) {
        printf("Error the server did not send the map \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (recv(socket_des, map, sizeof(map), 0) < 0) {
        printf("Error during recv of map \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    return ntohl(res.booknumber);
}

void new_book() {
    SocketMessagePreamble req, res;

    unsigned int booknumber = get_map(0);

    char lettera;
    int numero;
    int code_option;
    int option = -1;
    int counter_seats = 0; // uso questa variabile per contare quanti posti sono associati alla prenotazione in corso.

// procedura per lettura del posto
get_seat:

    printMap(map, booknumber);

redo_get_seat:

    // leggo il codice del posto (implemento un sistema che mi permette di leggere stringhe di vari tipi)
    code_option = getSeatNumber(&numero, &lettera);
    if (code_option == -1)
        goto redo_get_seat;
    else if (code_option == 1 || code_option == 2)
        goto exit_get_seat;

    // controllo che il posto sia libero
    if (map[lettera - 'A'][numero - 1] == 1) {
        printf("⚠️ You have already selected this place \n");
        goto redo_get_seat;
    }

    // chiedo al server se il posto è libero
    req.code = htons(3);
    req.row = htons(lettera - 'A');
    req.col = htons(numero - 1);
    req.booknumber = htonl(booknumber);

    printf("Check if seat is free... \n");

    if (send(socket_des, &req, sizeof(req), 0) < 0) {
        printf("Error: send of control free seat \n");
        goto redo_get_seat;
    }

    recv(socket_des, &res, sizeof(res), 0);

    if (ntohs(res.code) == 5) {
        printf("Error: this seat is alredy taken \n");
        goto redo_get_seat;
    } else if (ntohs(res.code) == 4) { // l'assegnazione temporanea del posto è andata a buon fine
        map[lettera - 'A'][numero - 1] = 1;
        counter_seats++;
    }

    goto get_seat;

exit_get_seat:

    req.row = 0;
    req.col = 0;
    req.dim = 0;
    req.booknumber = htonl(booknumber);

    if (code_option == 1) {
        req.code = htons(6);

        if (send(socket_des, &req, sizeof(req), 0) < 0) {
            printf("Error: send confirm book \n");
            return;
        }

        if (recv(socket_des, &res, sizeof(res), 0) < 0) {
            printf("Error: during reciving \n");
            return;
        }

        printf("Codice di conferma prenotazione-> %d \n", res.booknumber);

        HistoryRecord record;
        record.nseats = counter_seats;
        record.booknumber = booknumber;
        record.time = time(NULL);

        saveToHistory(&record);

    } else if (code_option == 2) { // caso in cui l'utente vuole annullare
        req.code = htons(7);

        if (send(socket_des, &req, sizeof(req), 0) < 0) {
            printf("Error: send cancel book \n");
            return;
        }
    }
}

void manage_old_book() {
    printHistory();

    char buff[255];
    unsigned int booknumber = 0;

get_book_number:
    printf("Book Number (0 to cancell) -> ");
    fflush(stdout);
    if (fgets(buff, sizeof(buff), stdin) < 0) {
        printf("Error fgets manage_old_book \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    booknumber = strtol(buff, 0, 10);

    if(booknumber == 0){
        return;
    }
    
    // qui faccio una verifica per capire se il codice di prenotazione inserito è un codice valido,
    // ma questa verifica dovrebbe variare a seconda di come viene generato il codice
    if (booknumber < 1782050448) {
        printf("⚠️ Invalid Book Number \n");
        fflush(stdout);
        goto get_book_number;
    }

    printf("Load book... \n");
    fflush(stdout);

    get_map(booknumber);
    printMap(map, booknumber);

get_old_book_opcode:
    printf("1 to delete book, 0 to cancell-> ");
    fflush(stdout);

    if (fgets(buff, sizeof(buff), stdin) < 0) {
        printf("Error fgets manage_old_book \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    int opcode = strtol(buff, 0, 10);
    if(opcode < 0 || opcode > 1){
        printf("Invalid operation \n");
        fflush(stdout);
        goto get_old_book_opcode;
    }


    if(opcode == 0){
        return;
    }

    if(opcode == 1){ // cancello la prenotazione

    }
}

void *thread_recv(void *arg){ // questo thread dovrà solo ricevere e scrivere all'interno di una pipe o in caso aggiornare la mappa
    
    return NULL;
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

    printf("Crete Recv thread \n");

    pthread_t recv_thread_id;
    pthread_create(&recv_thread_id, NULL, thread_recv, NULL);

    printf("Recv thread created ✅\n");

    printf("Open history file \n");

    history_des = open(HISTORY_NAME, O_CREAT | O_RDWR, 0666);

    printf("Setup complete ✅\n");

    char buffer[255];
    int op; // codice dell'operazione selezionato dall'utente all'interno del menu

    printMenu();

    while (1) {
        printf("-> ");
        fflush(stdout);

        fgets(buffer, sizeof(buffer), stdin);
        sscanf(buffer, "%d", &op); // estraggo il codice dell'operazione

        switch (op) {
        case 0:
            close(socket_des); // chiudo il socket
            printf("Exit success!\n");
            return 0;

        case 1:
            new_book();

            // alla fine ripulisco il terminale e ristampo il menu
            system("clear");
            printMenu();

            break;

        case 2:
            manage_old_book();
            
            // alla fine ripulisco il terminale e ristampo il menu
            // system("clear");
            printMenu();
            break;

        default:
            printf("⚠️  Codice operazione non valido\n");
            fflush(stdout);
            break;
        }

        op = -1;
    };
}