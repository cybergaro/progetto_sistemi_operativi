#include "unixfilemanager.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <unistd.h>

#include <stdio.h>

int create_map(char *fname) {
    int fd = open(fname, O_RDWR, 0644);

    if (fd < 0) {
        fd = open(fname, O_CREAT | O_RDWR, 0644);
        if (fd < 0) {
            return -1;
        }

        Seat posto_vuoto;
        posto_vuoto.flag = 0;
        posto_vuoto.nbook = 0;

        int total_seats = ROWS * COLS;

        for (int i = 0; i < total_seats; i++) {
            ssize_t bytes_written = write(fd, &posto_vuoto, sizeof(Seat));

            if (bytes_written != sizeof(Seat)) {
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

// questa funzione permette di avere una lista dei flag dei vari posti del cinema
// il parametro mask permete di fare in modo che tutti i flag diversi da 0 siano portati a 2 in
// modo che tutti i client sappiano che quel posto non è disponibile,
// a meno di quelli che hano un certo numero di prenotazione specificato in nbook

int get_all_flag(int ds, unsigned short int matrix[ROWS][COLS], short int mask, int nbook) {

    if (lseek(ds, 0, SEEK_SET) == -1) {
        return -1;
    }

    struct sembuf op;
    op.sem_flg = 0;

    Seat p;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {

            op.sem_num = r * COLS + c;
            op.sem_op = -1;

            if (semop(sems, &op, 1) == -1) {
                return -1;
            }

            ssize_t bytes_read = read(ds, &p, sizeof(Seat));
            if (bytes_read != sizeof(Seat)) {
                return -1;
            }

            op.sem_op = 1;
            if (semop(sems, &op, 1) == -1) {
                return -1;
            }

            if (mask == 1) {
                if (p.flag != 0) {
                    if (nbook != 0 && p.nbook == nbook) {
                        matrix[r][c] = 1;
                    } else {
                        matrix[r][c] = 2;
                    }
                }

            } else
                matrix[r][c] = p.flag;
        }
    }
    return 0;
}

int seat_set_flag(int ds, int row, int col, int flag_s, int nbook_v) {

    Seat s;
    s.flag = flag_s;

    if (s.flag == 0) {
        s.nbook = 0;
    } else {
        s.nbook = nbook_v;
    }
    off_t offset = (row * COLS + col) * sizeof(Seat);

    if (lseek(ds, offset, SEEK_SET) == (off_t)-1) {
        printf("Error on lseek seat_set_flag \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    struct sembuf op;
    op.sem_num = row * COLS + col;
    op.sem_flg = 0;
    op.sem_op = -1;

    if (semop(sems, &op, 1) < 0) {
        printf("Error on semop seat_set_flag \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (write(ds, &s, sizeof(Seat)) != sizeof(Seat)) {
        printf("Error on writing seat_set_flag \n");
        fflush(stdout);
        exit(EXIT_FAILURE);   
    }

    op.sem_op = 1;

    if (semop(sems, &op, 1) < 0) {
        printf("Error on semop seat_set_flag \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    fdatasync(ds);

    return 0;
}

int seat_get_flag(int ds, int row, int col) {
    Seat s;

    off_t offset = (row * COLS + col) * sizeof(Seat);

    if (lseek(ds, offset, SEEK_SET) == (off_t)-1) {
        return -1;
    }

    struct sembuf buf;
    buf.sem_flg = 0;
    buf.sem_num = row * COLS + col;
    buf.sem_op = -1;

    if (semop(sems, &buf, 1) < 0) {
        return -1;
    }

    ssize_t bytes_letti = read(ds, &s, sizeof(Seat));

    buf.sem_op = 1;

    if (semop(sems, &buf, 1) < 0) {
        return -1;
    }

    if (bytes_letti != sizeof(Seat)) {
        return -1;
    }

    return s.flag;
}

int set_all_flag_from_nbook(int ds, int flag, int nbook) {

    Seat s;

    if (lseek(ds, 0, SEEK_SET) < 0) {
        printf("Error lseek \n");
        fflush(stdout);
        return -1;
    }

    struct sembuf buf;
    buf.sem_flg = 0;

    for (int i = 0; i < ROWS * COLS; i++) {

        buf.sem_num = i;
        buf.sem_op = -1;

        if (semop(sems, &buf, 1) < 0) {
            printf("Semop error \n");
            fflush(stdout);
            return -1;
        }

        if (read(ds, &s, sizeof(Seat)) != sizeof(Seat)) {
            return -1;
        }

        if (s.nbook == nbook) {
            printf("corrispondenza trovata \n");
            fflush(stdout);

            s.flag = flag;

            if (flag == 0) {
                s.nbook = 0;
            }

            if (lseek(ds, -sizeof(Seat), SEEK_CUR) == (off_t)-1) {
                printf("Error lseek \n");
                fflush(stdout);
                return -1;
            }

            if (write(ds, &s, sizeof(Seat)) != sizeof(Seat)) {
                printf("Error write \n");
                fflush(stdout);
                return -1;
            }
        }

        buf.sem_op = 1;

        if (semop(sems, &buf, 1) < 0) {
            printf("Semop error \n");
            fflush(stdout);
            return -1;
        }
    }
    
    fdatasync(ds);

    return 0;
}