#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define RIGHE 6
#define COLONNE 10

int main(int argc, char const *argv[])
{
    int op1, op2;
    unsigned char mat[RIGHE][COLONNE];

    int fd = open("seat.bin", O_RDWR | O_CREAT);    

    while(1){
        read(fd, &mat, sizeof(mat));

        for (int i = 0; i < RIGHE; i++){
            for (int j = 0; j < COLONNE; j++){
                printf("%2d ", mat[i][j]);
            }
            printf("\n");
        }

        scanf("%d %d", &op1, &op2);
        if(op1 == -1){
            return -1;
        }

        read(fd, &mat, sizeof(mat));

        mat[op1][op2] = 10;

        lseek(fd, 0, 0);
        if(write(fd, &mat, sizeof(mat)) == -1){
            printf("Errore nel salvataggio \n");
        }


    }

    return 0;
}
