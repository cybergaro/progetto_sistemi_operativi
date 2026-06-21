# 📽️🍿 Sistema di Prenotazione Posti Cinema

Sistema **client-server concorrente** per la gestione e prenotazione in tempo reale dei posti in una sala cinematografica.

- 🏛️ **150 posti** — 10 file (`A`–`J`) × 15 colonne
- 🔀 **Multithreading POSIX** — gestione simultanea di più client
- 🔒 **Fine-grained locking** — mutex per singolo posto, massimo parallelismo
- 🖥️ **Doppia interfaccia client** — CLI con ANSI colors e GUI GTK4
- 🌐 **Comunicazione TCP** — protocollo binario custom con byte ordering

---

## 📖 Documentazione

Per l'architettura completa, il protocollo di comunicazione, i meccanismi di sincronizzazione e la guida alla compilazione, consulta:

### 👉 [DOCS.md](./DOCS.md)

---

## ⚡ Quick Start

**Server:**
```bash
cd server && make posix && ./a.out
```

**Client CLI:**
```bash
cd client && make posix && ./a.out
```

**Client GUI (GTK4):**
```bash
cd client && make macos-gui && ./a.out
```