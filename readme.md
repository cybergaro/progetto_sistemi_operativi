Progetto SO Francesco Garofolo && Flavio Favero
# Sistema di prenotazione posti 📽️🍿
## Consegna
Realizzazione di un sistema di prenotazione posti per una sala
cinematografica. Un processo su una macchina server gestisce una mappa di
posti per una sala cinematografica. Ciascun posto e' caratterizzato da un
numero di fila, un numero di poltrona ed un FLAG indicante se il posto
e' gia' stato prenotato o meno.
Il server accetta e processa le richieste di prenotazione
di posti da uno o piu' client (residenti, in generale, su macchine diverse).
Un client deve fornire ad un utente le seguenti funzioni:
- Visualizzare la mappa dei posti in modo da individuare quelli ancora
disponibili.
- Inviare al server l'elenco dei posti che si intende prenotare (ciascun
posto da prenotare viene ancora identificato tramite numero di fila e numero di
poltrona).
- Attendere dal server la conferma di effettuata prenotazione ed un codice di prenotazione.
- Disdire una prenotazione per cui si possiede un codice.

Si precisa che lo studente e' tenuto a realizzare sia il client che il
server.

Il server deve poter gestire le richieste dei client in modo concorrente. 
                 
## Istruzioni
### 🖥️ Server
Il server è compatibile **solo con sistemi POSIX** (Linux, macOS). <br>
Per compilare ed eseguire, apri il terminale e digita:
```bash
cd server
make posix
./server.out
```

### 💻 Client
Il client è cross-platform e compatibile sia con sistemi **POSIX** che **Windows**. <br >
Su sistemi POSIX:
```bash
cd client
make posix
./client.out
```
Su sistemi Windows:
```DOS
cd client
make windows
./client.exe
```

## Documentazione
La documentazione completa del progetto è disponibile [qui](https://docs.google.com/document/d/1iBMJ6MiyjOpn6gtsE4-hWcNd_O7Nk7WY0Damla6CSV0/edit?usp=sharing)