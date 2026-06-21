# 📽️🍿 Sistema di Prenotazione Posti Cinema

Sistema client-server concorrente per la gestione e prenotazione in tempo reale dei posti in una sala cinematografica.

---

## 📋 Indice

- [Introduzione](#introduzione)
- [Architettura del Sistema](#architettura-del-sistema)
- [Protocollo di Comunicazione](#protocollo-di-comunicazione)
- [Meccanismi di Sincronizzazione](#meccanismi-di-sincronizzazione)
- [Struttura del Codice](#struttura-del-codice)
- [Compilazione ed Esecuzione](#compilazione-ed-esecuzione)

---

## Introduzione

La sala è strutturata come una griglia composta da **10 file** (identificate dalle lettere `A`–`J`) e **15 colonne** (poltrone numerate da `1` a `15`), per un totale di **150 posti** gestiti centralmente.

L'applicazione consente a molteplici client remoti di:

- Consultare lo stato della sala in tempo reale
- Selezionare e bloccare temporaneamente i posti (*pending*)
- Completare l'acquisto o annullare la prenotazione

Il tutto garantendo consistenza dello stato e prevenendo doppie prenotazioni (*race conditions*).

---

## Architettura del Sistema

Il sistema adotta un'architettura **Client-Server** basata su IPv4, con comunicazione via **TCP (`SOCK_STREAM`)** per una trasmissione affidabile e ordinata.

### Modello di Concorrenza del Server

Il server utilizza **Multithreading POSIX (`pthread`)** per gestire più client simultaneamente:

1. Il ciclo principale rimane in ascolto sulla porta `8080` tramite `accept()`
2. Ad ogni nuova connessione, viene istanziato un thread dedicato con `pthread_create()`
3. Il thread esegue la routine `connection_handler()`, isolando la logica di ogni sessione
4. Il thread principale torna immediatamente in ascolto di nuove connessioni

### Persistenza dei Dati e I/O su File

Lo stato della sala è persistito nel file binario `cinema_map.bin`, contenente **150 record consecutivi** di tipo `Seat`:

```c
typedef struct {
    unsigned short int flag;   /* 0 = Libero | 1 = Pending | 2 = Confermato */
    unsigned int nbook;        /* Codice univoco della prenotazione */
} Seat;
```

L'accesso alle singole posizioni avviene calcolando l'offset binario con `lseek()`:

```c
off_t offset = (row * COLS + col) * sizeof(Seat);
```

Ogni scrittura è seguita da `fdatasync(ds)` per garantire la persistenza fisica dei dati anche in caso di crash.

---

## Protocollo di Comunicazione

Lo scambio di messaggi è codificato mediante la struttura a dimensione fissa `SocketMessagePreamble`:

```c
typedef struct {
    unsigned short code;       /* Codice identificativo della richiesta/risposta */
    unsigned short row;        /* Indice della fila */
    unsigned short col;        /* Indice della poltrona */
    unsigned int booknumber;   /* ID della prenotazione/sessione */
    unsigned int dim;          /* Dimensione in byte del payload opzionale */
} SocketMessagePreamble;
```

### Tabella dei Codici Operazione

| Codice | Descrizione | Direzione |
|:------:|-------------|-----------|
| `1` | Richiesta Mappa Posti | Client → Server |
| `2` | Invio Mappa Posti | Server → Client (payload = matrice dei flag) |
| `3` | Richiesta Assegnazione Posto | Client → Server (specifica `row` e `col`) |
| `4` | Posto Riservato (Pending) | Server → Client (esito positivo, `flag = 1`) |
| `5` | Posto Non Prenotabile | Server → Client (posto già occupato o in conflitto) |
| `6` | Conferma Prenotazione | Client → Server (posti pending → `flag = 2`) |
| `7` | Cancella Prenotazione | Client → Server (posti pending → `flag = 0`) |
| `8` | Invia Nuovo Booknumber | Server → Client (nuovo ID per una nuova transazione) |

> [!WARNING]
> **Byte Ordering:** Tutti i campi numerici del `Preamble` vengono convertiti in formato di rete prima dell'invio (`htons()` / `htonl()`) e riconvertiti alla ricezione (`ntohs()` / `ntohl()`), garantendo compatibilità tra architetture Big-Endian e Little-Endian.

---

## Meccanismi di Sincronizzazione

### Sincronizzazione dell'Elenco Client

L'array dei socket attivi (modulo `socketmanagment`) è protetto da un mutex globale `lock`:

```c
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
```

### Fine-Grained Locking sui Posti

Per evitare colli di bottiglia, ogni posto ha il **proprio mutex indipendente**:

```c
pthread_mutex_t seat_mutexes[ROWS * COLS];
```

Quando un client richiede un posto (codice `3`), il thread acquisisce **solo il mutex di quella specifica coordinata**:

```
mutex index = row * COLS + col
```

Questo consente il pieno parallelismo su posti diversi, con contesa solo in caso di conflitto sul medesimo posto.

---

## Struttura del Codice

```
.
├── server/
│   ├── main.c                              # Setup rete, gestione segnali (SIGINT, SIGPIPE), loop client
│   ├── filemanager/
│   │   └── unixfilemanager.c               # I/O su file binario, masking posti pending
│   ├── socketmanagment/
│   │   └── socketmanagment.c               # Gestione dinamica dei descrittori socket
│   └── utility/
│       └── utility.c                       # Generazione booknumber univoco via time(NULL)
│
└── client/
    ├── main.c                              # Interfaccia CLI con escape ANSI, input "A1"/"C12"
    ├── gui/                                # Interfaccia GTK4 con toggle buttons e CSS runtime
    └── history.bin                         # Storico locale prenotazioni (timestamp, codice, n° posti)
```

### Moduli Server

| Modulo | Descrizione |
|--------|-------------|
| `main.c` | Configura `sockaddr_in`, `socket()`, `bind()`, `listen()`. Gestisce `SIGINT` e `SIGPIPE` |
| `unixfilemanager.c` | Lettura/scrittura flag su file; i posti *pending* appaiono come "selezionati" al proprietario e "occupati" agli altri |
| `socketmanagment.c` | Array dinamico dei socket con riallocazione (`realloc`) |
| `utility.c` | ID di sessione univoco generato con `time(NULL)` |

### Moduli Client

| Modalità | Descrizione |
|----------|-------------|
| **CLI** | Griglia interattiva con colori ANSI; input coordinate testuali (`sscanf`) |
| **GUI** | Applicazione GTK4 con pannello di toggle button; stile gestito via CSS iniettato a runtime |
| **History** | File binario locale `history.bin` con storico delle prenotazioni confermate |

---

## Compilazione ed Esecuzione

### Server

```bash
cd server
make posix
./a.out
```

> Il flag `-lpthread` viene applicato automaticamente dal Makefile per il linking della libreria dei thread.

### Client — Modalità CLI

```bash
cd client
make posix
./a.out
```

### Client — Modalità GUI (GTK4)

```bash
cd client
make macos-gui
./a.out
```

> [!NOTE]
> La modalità GUI richiede un ambiente compatibile con GTK4 (es. macOS con i pacchetti di sviluppo GTK4 installati).