#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

void printhelp();

void printhelp() {
    fprintf(stderr, "usage: shmdmp [shmid]\n");
    fprintf(stderr, "try using ipcs -m get the an id of a segment to dump\n");
}

int main(int argc, char *argv[]) {
    int shmid;
    char *data;
    char *failureptr;
    struct shmid_ds buf;
    unsigned long i = 0;

    if (argc > 2) {
        printhelp();
        exit(1);
    }

    /* make the key: */
    shmid = strtol(argv[1], &failureptr, 10);
    if (failureptr == argv[1]) {
        perror("strtol");
        printhelp();
        exit(1);
    }

    /* attach to the segment to get a pointer to it: */
    data = shmat(shmid, (void *) 0, SHM_RDONLY);
    if (data == (char *) (-1)) {
        perror("shmat");
        exit(1);
    }

    if (shmctl(shmid, IPC_STAT, &buf) == -1) {
        perror("shmctl");
        exit(1);
    }

    for (i = 0; i < buf.shm_segsz; i++) {
        printf("%c", data[i]);
    }

    /* detach from the segment: */
    shmdt(data);

    return 0;
}
