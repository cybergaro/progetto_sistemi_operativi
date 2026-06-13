#include "unixfilemanager.h"

int fd = open(MAP_FILE, O_RDWR, 0644);

    if (fd < 0){ // caso in cui il file non esiste
        fd = open(MAP_FILE, O_CREAT | O_TRUNC | O_RDWR, 0644);
        if(fd < 0){
            printf("Error map file creation\n");
            exit(EXIT_FAILURE);
        }
    }