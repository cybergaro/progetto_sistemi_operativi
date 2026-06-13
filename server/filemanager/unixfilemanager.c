#include "unixfilemanager.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int create_map(char *fname) {
    int fd = open(fname, O_RDWR, 0644);

    if (fd < 0) {
        fd = open(fname, O_CREAT | O_RDWR, 0644);
        if (fd < 0) {
            printf("Errore nella creazione della mappa\n");
            return -1;
        }

        Posto posto_vuoto;
        posto_vuoto.occupato = 0;
        posto_vuoto.num_prenotazione = 0;

        int total_seats = ROWS * COLS;

        for (int i = 0; i < total_seats; i++) {
            ssize_t bytes_written = write(fd, &posto_vuoto, sizeof(Posto));

            if (bytes_written != sizeof(Posto)) {
                perror("Errore durante l'inizializzazione dei posti");
                close(fd);
                return -1;
            }
        }

        close(fd);
        return 0;
    }

    close(fd);
    return 0;
}