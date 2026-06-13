#include "unixfilemanager.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
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

int get_all_flag(int ds, unsigned short int matrix[ROWS][COLS], short int mask) {

    if (lseek(ds, 0, SEEK_SET) == -1) {
        return -1;
    }
    Seat p;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            ssize_t bytes_read = read(ds, &p, sizeof(Seat));
            if (bytes_read != sizeof(Seat)) {
                return -1;
            }
            if (mask == 1) {
                if (p.flag != 0) {
                    matrix[r][c] = 2;
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
        return -1;
    }
    if (write(ds, &s, sizeof(Seat)) != sizeof(Seat)) {
        return -1;
    }
    return 0;
}

int seat_get_flag(int ds, int row, int col) {
    Seat s;

    off_t offset = (row * COLS + col) * sizeof(Seat);

    if (lseek(ds, offset, SEEK_SET) == (off_t)-1) {
        printf("errore lseek \n");
        return -1;
    }
    ssize_t bytes_letti = read(ds, &s, sizeof(Seat));

    if (bytes_letti != sizeof(Seat)) {
        printf("errore byte letti \n");
        return -1;
    }

    return s.flag;
}