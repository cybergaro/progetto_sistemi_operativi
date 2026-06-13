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


int get_all_flag(int ds, unsigned short int matrix[ROWS][COLS],short int mask)
{
   
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
            if(mask==1){
                if (p.flag!=0)
                {
                    matrix[r][c] = 2; 
                }
                
            }
            else matrix[r][c] = p.flag;
            
        }
    }
    return 0;
}