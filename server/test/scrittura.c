#include <stdio.h>

// unix library
#include <fcntl.h>
#include <unistd.h>

#define RIGHE 6
#define COLONNE 10

int main(int argc, char const *argv[])
{

    unsigned char mia_variabile[RIGHE][COLONNE];

    for (int i = 0; i < RIGHE; i++){
        for (int j = 0; j < COLONNE; j++){
            mia_variabile[i][j] = 0;
        }
    }

    int fd = open("seat.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        write(STDERR_FILENO, "Errore nell'apertura del file\n", 30);
        return 1;
    }

    ssize_t bytes_scritti = write(fd, &mia_variabile, sizeof(mia_variabile));

    if (bytes_scritti == -1) {
        write(STDERR_FILENO, "Errore durante la scrittura\n", 28);
        close(fd); // Chiudi sempre il file prima di uscire!
        return 1;
    }

    if (close(fd) == -1) {
        write(STDERR_FILENO, "Errore nella chiusura del file\n", 31);
        return 1;
    }

    return 0;
}
