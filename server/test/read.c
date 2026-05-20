#include <stdio.h>

// unix library
#include <fcntl.h>
#include <unistd.h>

#define RIGHE 6
#define COLONNE 10

int main(int argc, char const *argv[])
{

    unsigned char mia_variabile[RIGHE][COLONNE];

    int fd = open("variabile.bin", O_RDONLY, 0644);

    if (fd == -1) {
        write(STDERR_FILENO, "Errore nell'apertura del file\n", 30);
        return 1;
    }

    read(fd, &mia_variabile, sizeof(mia_variabile));

    if(close(fd) == -1){
    }

    for (int i = 0; i < RIGHE; i++){
        for (int j = 0; j < COLONNE; j++){
            printf("%2d ",mia_variabile[i][j]);
        }
        printf("\n");
    }

    fflush(stdout);

    return 0;
}
