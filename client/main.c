#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32) || defined(WINDOWS)
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "utility.h"

#define SERVER_ADDR "192.168.1.36"
#define SERVER_PORT 8080

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
 * 2)  send map with flags
 * 3)  request seat
 * 4)  reserved seat
 * 5)  seat not bookable
 * 6)  confirm book
 * 7)  cancell book
 * 8)  send booknumber
 * 9)  remove single seat during the booking operation
 * 10) broadcast single seat update
 */

unsigned short int map[ROWS][COLS]; // matrice che rappresenta lo stato di occupazione dei posti

#if defined(_WIN32) || defined(WINDOWS)

// ===========================================================================
// ================================= Windows =================================
// ===========================================================================

SOCKET socket_des;
HANDLE pipeRead, pipeWrite;

short int rewrite_map = 0;
CRITICAL_SECTION rewrite_map_mutex;

unsigned int get_map(unsigned int booknumber) {
    SocketMessagePreamble req, res;

    req.code = htons(1);
    req.row = 0;
    req.col = 0;
    req.dim = 0;
    req.booknumber = booknumber != 0 ? htonl(booknumber) : 0;

    if (send(socket_des, (const char*)&req, sizeof(req), 0) < 0) {
        printf("Error: request of cinema map not found \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    DWORD bytesRead;
    if (!ReadFile(pipeRead, &res, sizeof(res), &bytesRead, NULL) || bytesRead == 0) {
        printf("Error during recv preamble \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (ntohs(res.code) != 2 || ntohl(res.dim) != sizeof(map)) {
        printf("Error the server did not send the map \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (!ReadFile(pipeRead, map, sizeof(map), &bytesRead, NULL) || bytesRead == 0) {
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
    int counter_seats = 0;
    DWORD bytesRead;

    EnterCriticalSection(&rewrite_map_mutex);
    rewrite_map = 1;
    LeaveCriticalSection(&rewrite_map_mutex);

get_seat:
    system("cls");
    printMap(map, booknumber);

redo_get_seat:
    code_option = getSeatNumber(&numero, &lettera);
    if (code_option == -1)
        goto redo_get_seat;
    else if (code_option == 1 || code_option == 2)
        goto exit_get_seat;

    if (map[lettera - 'A'][numero - 1] == 1) { 
        req.code = htons(9);
        req.row = htons(lettera - 'A');
        req.col = htons(numero - 1);
        req.booknumber = htonl(booknumber);

        printf("Sending to server the removing seat\n");
        map[lettera - 'A'][numero - 1] = 0;

        if (send(socket_des, (const char*)&req, sizeof(req), 0) < 0) {
            printf("Error sending removal request");
            exit(EXIT_FAILURE);
        }

        counter_seats--;
        goto get_seat;
    }

    req.code = htons(3);
    req.row = htons(lettera - 'A');
    req.col = htons(numero - 1);
    req.booknumber = htonl(booknumber);

    printf("Check if seat is free... \n");

    if (send(socket_des, (const char*)&req, sizeof(req), 0) < 0) {
        printf("Error: send of control free seat \n");
        goto redo_get_seat;
    }

    ReadFile(pipeRead, &res, sizeof(res), &bytesRead, NULL);

    if (ntohs(res.code) == 5) {
        printf("Error: this seat is alredy taken \n");
        goto redo_get_seat;
    } else if (ntohs(res.code) == 4) {
        map[lettera - 'A'][numero - 1] = 1;
        counter_seats++;
    }

    goto get_seat;

exit_get_seat:
    EnterCriticalSection(&rewrite_map_mutex);
    rewrite_map = 0;
    LeaveCriticalSection(&rewrite_map_mutex);

    req.row = 0;
    req.col = 0;
    req.dim = 0;
    req.booknumber = htonl(booknumber);

    if (code_option == 1) {
        if (counter_seats == 0) return;

        req.code = htons(6);

        if (send(socket_des, (const char*)&req, sizeof(req), 0) < 0) {
            printf("Error: send confirm book \n");
            return;
        }

        if (!ReadFile(pipeRead, &res, sizeof(res), &bytesRead, NULL) || bytesRead == 0) {
            printf("Error: during reciving \n");
            return;
        }

        printf("Codice di conferma prenotazione-> %d \n", res.booknumber);

        HistoryRecord record;
        record.nseats = counter_seats;
        record.booknumber = booknumber;
        record.time = time(NULL);

        saveToHistory(&record);

    } else if (code_option == 2) { 
        req.code = htons(7);
        if (send(socket_des, (const char*)&req, sizeof(req), 0) < 0) {
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
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        printf("Error fgets manage_old_book \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    booknumber = strtol(buff, 0, 10);
    if (booknumber == 0) {
        return;
    }

    if (booknumber < 1782050448) {
        printf("⚠️ Invalid Book Number \n");
        fflush(stdout);
        goto get_book_number;
    }

    printf("Load book... \n");
    fflush(stdout);

    get_map(booknumber);
    system("cls");
    printMap(map, booknumber);

get_old_book_opcode:
    printf("1 to delete book, 0 to cancell-> ");
    fflush(stdout);

    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        printf("Error fgets manage_old_book \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    int opcode = strtol(buff, 0, 10);
    if (opcode < 0 || opcode > 1) {
        printf("Invalid operation \n");
        fflush(stdout);
        goto get_old_book_opcode;
    }

    if (opcode == 0) {
        return;
    }

    if (opcode == 1) { 
        SocketMessagePreamble req;
        req.row = 0; req.col = 0; req.dim = 0;
        req.code = htons(7);
        req.booknumber = htonl(booknumber);

        if (send(socket_des, (const char*)&req, sizeof(req), 0) < 0) {
            printf("Error: send cancel book \n");
            return;
        }

        removeFromHistory(booknumber);
        printf("Book deleted \n");
    }
}

void async_update_map(short int row, short int col, short int flag) {
    map[row][col] = flag;

    EnterCriticalSection(&rewrite_map_mutex);
    if (rewrite_map == 1) {
        printf("\0337");
        printf("\033[H");
        printMap(map, 0);
        printf("\0338");
        fflush(stdout);
    }
    LeaveCriticalSection(&rewrite_map_mutex);
}

DWORD WINAPI thread_recv(LPVOID arg) {
    SocketMessagePreamble buff;
    char extra_buff[1024]; 
    DWORD bytesWritten;

    while (1) {
        int bytes_ricevuti = recv(socket_des, (char*)&buff, sizeof(buff), MSG_WAITALL);

        if (bytes_ricevuti == 0) {
            printf("chiusura connessione\n");
            break;
        } else if (bytes_ricevuti < 0) {
            break;
        }

        if (ntohs(buff.code) == 10) { 
            async_update_map(ntohs(buff.row), ntohs(buff.col), (short int)ntohl(buff.short_data));
            continue;
        }

        if (!WriteFile(pipeWrite, &buff, sizeof(buff), &bytesWritten, NULL)) {
            printf("errore di scrittura pipe\n");
            exit(EXIT_FAILURE);
        }

        if (buff.dim == 0) 
            continue;

        bytes_ricevuti = recv(socket_des, extra_buff, ntohl(buff.dim), MSG_WAITALL);
        if (bytes_ricevuti <= 0) break;

        if (!WriteFile(pipeWrite, extra_buff, bytes_ricevuti, &bytesWritten, NULL)) {
            printf("errore di scrittura pipe\n");
        }
    }
    return 0;
}

int main(int argc, char const *argv[]) {

    printf("Setup... ⏳\n");
    
    // abilito i colori per il terminale
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }

    printf("Socket setup \n");

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Error: WSAStartup failed.\n");
        exit(EXIT_FAILURE);
    }

    socket_des = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_des == INVALID_SOCKET) {
        printf("Error: Creation of socket \n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    if (connect(socket_des, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Error: Connection to server \n");
        closesocket(socket_des);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server ✅\n");
    printf("create pipe\n");

    if (!CreatePipe(&pipeRead, &pipeWrite, NULL, 0)) {
        printf("error creating the pipe\n");
        closesocket(socket_des);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("pipe created\n");
    printf("Creation of mutex for rewrite map...\n");

    InitializeCriticalSection(&rewrite_map_mutex);
    printf("Mutex for rewrite map created \n");

    printf("Create recv thread... \n");

    HANDLE thread_recv_id = CreateThread(NULL, 0, thread_recv, NULL, 0, NULL);
    if (thread_recv_id == NULL) {
        printf("Error thread creation\n");
        exit(EXIT_FAILURE);
    }

    printf("Recv thread created  ✅\n");
    printf("Open history file \n");

    history_des = CreateFileA(HISTORY_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    printf("Setup complete ✅\n");

    char buffer[255];
    int op;

    printMenu();

    while (1) {
        printf("-> ");
        fflush(stdout);

        fgets(buffer, sizeof(buffer), stdin);
        sscanf(buffer, "%d", &op);

        switch (op) {
        case 0:
            closesocket(socket_des); 
            WSACleanup();
            CloseHandle(history_des);
            printf("Exit success!\n");
            return 0;

        case 1:
            new_book();
            system("cls");
            printMenu();
            break;

        case 2:
            manage_old_book();
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

#else

// ===========================================================================
// ================================== POSIX ==================================
// ===========================================================================

int socket_des;
int pipefd[2]; // pipe per comunicare con il server

short int rewrite_map = 0;
pthread_mutex_t rewrite_map_mutex; // questa mutex indica se la mappa deve essere ristampata in modo asincrono

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

    if (read(pipefd[0], &res, sizeof(res)) < 0) {
        printf("Error during recv preamble \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (ntohs(res.code) != 2 || ntohl(res.dim) != sizeof(map)) {
        printf("Error the server did not send the map \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (read(pipefd[0], map, sizeof(map)) < 0) {
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

    // abilito la ristampa della mappa asincrona
    pthread_mutex_lock(&rewrite_map_mutex);
    rewrite_map = 1;
    pthread_mutex_unlock(&rewrite_map_mutex);

// procedura per lettura del posto
get_seat:
    system("clear");
    printMap(map, booknumber);

redo_get_seat:

    // leggo il codice del posto (implemento un sistema che mi permette di leggere stringhe di vari tipi)
    code_option = getSeatNumber(&numero, &lettera);
    if (code_option == -1)
        goto redo_get_seat;
    else if (code_option == 1 || code_option == 2)
        goto exit_get_seat;

    if (map[lettera - 'A'][numero - 1] == 1) { // controlo che il posto che ho selezionato non sia uno di quelli che avevo selezionato in precedenza
        // chiedo al server di cancellare il posto che ho selezionato durante la prenotazione
        req.code = htons(9);
        req.row = htons(lettera - 'A');
        req.col = htons(numero - 1);
        req.booknumber = htonl(booknumber);

        printf("Sending to server the removing seat\n");
        map[lettera - 'A'][numero - 1] = 0;

        if (send(socket_des, &req, sizeof(req), 0) < 0) {
            printf("Error sending removal request");
            exit(EXIT_FAILURE);
        }

        counter_seats--;
        goto get_seat;
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

    read(pipefd[0], &res, sizeof(res));

    if (ntohs(res.code) == 5) {
        printf("Error: this seat is alredy taken \n");
        goto redo_get_seat;
    } else if (ntohs(res.code) == 4) { // l'assegnazione temporanea del posto è andata a buon fine
        map[lettera - 'A'][numero - 1] = 1;
        counter_seats++;
    }

    goto get_seat;

exit_get_seat:

    // disabilitto la ristampa della mappa asincrona
    pthread_mutex_lock(&rewrite_map_mutex);
    rewrite_map = 0;
    pthread_mutex_unlock(&rewrite_map_mutex);

    req.row = 0;
    req.col = 0;
    req.dim = 0;
    req.booknumber = htonl(booknumber);

    if (code_option == 1) {

        if (counter_seats == 0)
            return; // se non ci sono posti

        req.code = htons(6);

        if (send(socket_des, &req, sizeof(req), 0) < 0) {
            printf("Error: send confirm book \n");
            return;
        }

        if (read(pipefd[0], &res, sizeof(res)) < 0) {
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

    if (booknumber == 0) {
        return;
    }

    if (booknumber < 1782050448) {
        printf("⚠️ Invalid Book Number \n");
        fflush(stdout);
        goto get_book_number;
    }

    printf("Load book... \n");
    fflush(stdout);

    get_map(booknumber);
    system("clear");
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
    if (opcode < 0 || opcode > 1) {
        printf("Invalid operation \n");
        fflush(stdout);
        goto get_old_book_opcode;
    }

    if (opcode == 0) {
        return;
    }

    if (opcode == 1) { // cancello la prenotazione
        SocketMessagePreamble req;
        req.row = 0;
        req.col = 0;
        req.dim = 0;
        req.code = htons(7);
        req.booknumber = htonl(booknumber);

        if (send(socket_des, &req, sizeof(req), 0) < 0) {
            printf("Error: send cancel book \n");
            return;
        }

        removeFromHistory(booknumber);

        printf("Book deleted \n");
    }
}

void async_update_map(short int row, short int col, short int flag) { // questa funzione si occupa di aggiornare lo stato di un posto all'interno della mappa in modo asyncrono
    map[row][col] = flag;

    pthread_mutex_lock(&rewrite_map_mutex);

    if (rewrite_map == 1) {
        printf("\0337");
        printf("\033[H");
        printMap(map, 0);
        printf("\0338");

        fflush(stdout);
    }

    pthread_mutex_unlock(&rewrite_map_mutex);
}

void *thread_recv(void *arg) { // thread usato per fare il recv e inoltro su una pipe e aggiornamento mappa.
    SocketMessagePreamble buff;
    char extra_buff[1024]; // viene utilizzato per i dati extra

    while (1) {
        int bytes_ricevuti = recv(socket_des, &buff, sizeof(buff), MSG_WAITALL);

        if (bytes_ricevuti == 0) {
            printf("chiusura connessione\n");
            break;
        } else if (bytes_ricevuti < 0) {
            break;
        }

        if (ntohs(buff.code) == 10) { 
            async_update_map(ntohs(buff.row), ntohs(buff.col), (short int)ntohl(buff.short_data));
            continue;
        }

        if (write(pipefd[1], &buff, sizeof(buff)) < 0) {
            printf("errore di scrittura\n");
            exit(EXIT_FAILURE);
        }

        if (buff.dim == 0) 
            continue;

        bytes_ricevuti = recv(socket_des, &extra_buff, ntohl(buff.dim), MSG_WAITALL);

        if (bytes_ricevuti == 0) {
            printf("chiusura connessione\n");
            break;
        } else if (bytes_ricevuti < 0) {
            printf("errore di scrittura dal socket\n");
            break;
        }

        if (write(pipefd[1], &extra_buff, bytes_ricevuti) < 0) {
            printf("errore di scrittura\n");
        }
    }

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
    printf("create pipe\n");

    if (pipe(pipefd) < 0) {
        printf("error creating the pipe\n");
        close(socket_des);
        exit(EXIT_FAILURE);
    }

    printf("pipe created\n");
    printf("Creation of mutex for rewrite map...\n");

    if (pthread_mutex_init(&rewrite_map_mutex, NULL) < 0) {
        printf("Error during creation of mutex \n");
        exit(EXIT_FAILURE);
    }

    printf("Mutex for rewrite map created \n");
    printf("Create recv thread... \n");

    pthread_t thread_recv_id;
    pthread_create(&thread_recv_id, NULL, thread_recv, NULL);

    printf("Recv thread created  ✅\n");
    printf("Open history file \n");

    history_des = open(HISTORY_NAME, O_CREAT | O_RDWR, 0666);

    printf("Setup complete ✅\n");

    char buffer[255];
    int op; 

    printMenu();

    while (1) {
        printf("-> ");
        fflush(stdout);

        fgets(buffer, sizeof(buffer), stdin);
        sscanf(buffer, "%d", &op); 

        switch (op) {
        case 0:
            close(socket_des); 
            printf("Exit success!\n");
            return 0;

        case 1:
            new_book();
            system("clear");
            printMenu();
            break;

        case 2:
            manage_old_book();
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

#endif