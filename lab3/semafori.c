#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define PrazanStol 0
#define KRUH_SUNKA 1
#define SUNKA_SIR 2
#define KRUH_SIR 3

int SemId;
int *RedniBrojKupca; // dijeljena memorija
int ShmId;


void SemGet(int n) {
    SemId = semget(IPC_PRIVATE, n, 0600 | IPC_CREAT);
    if (SemId == -1) {
        perror("semget");
        exit(1);
    }
}

void SemSet(int SemNum, int SemVal) {
    union semun {
        int val;
    } arg;
    arg.val = SemVal;
    if (semctl(SemId, SemNum, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        exit(1);
    }
}

int SemOp(int SemNum, int SemOp) {
    struct sembuf SemBuf;
    SemBuf.sem_num = SemNum;
    SemBuf.sem_op = SemOp;
    SemBuf.sem_flg = 0;
    return semop(SemId, &SemBuf, 1);
}

void SemRemove(void) {
    semctl(SemId, 0, IPC_RMID, 0);
}

void kupac(int tip) {
    int mojRedniBroj = __sync_fetch_and_add(RedniBrojKupca, 1);
	//printf("pokrenut kupac tipa %d i broja %d\n",tip, mojRedniBroj);
	switch (tip) {
	    case 1:
	        SemOp(KRUH_SUNKA, -1);
	        printf("Kupac %d (ima SUNKA) uzima KRUH i SIR\n", mojRedniBroj);
			pokreni_kupca(1);
	        SemOp(PrazanStol, 1);
	        break;
	    case 2:
	        SemOp(KRUH_SIR, -1);
	        printf("Kupac %d (ima SIR) uzima KRUH i SUNKA\n", mojRedniBroj);
			pokreni_kupca(2);
	        SemOp(PrazanStol, 1);
	        break;
	    case 3:
	        SemOp(SUNKA_SIR, -1);
	        printf("Kupac %d (ima KRUH) uzima SIR i SUNKA\n", mojRedniBroj);
			pokreni_kupca(3);
	        SemOp(PrazanStol, 1);
	        break;
	}
    
}

void trgovac() {
    srand(getpid());
    while (1) {
        SemOp(PrazanStol, -1);
        int sastojak = rand() % 3;

        sleep(1);

        switch (sastojak) {
            case 0:
                printf("Trgovac stavlja: KRUH i SUNKA\n");
                SemOp(KRUH_SIR, 1);
                break;
            case 1:
                printf("Trgovac stavlja: KRUH i SIR\n");
                SemOp(KRUH_SUNKA, 1);
                break;
            case 2:
                printf("Trgovac stavlja: SUNKA i SIR\n");
                SemOp(SUNKA_SIR, 1);
                break;
        }
    }
}

void pokreni_kupca(int tip) {
    if (fork() == 0) {
        kupac(tip);
        exit(0);
    }
}



int main() {
    srand(time(NULL));

    // dijeljena memorija za redni broj
    ShmId = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0600);
    if (ShmId == -1) {
        perror("shmget");
        exit(1);
    }

    RedniBrojKupca = (int *)shmat(ShmId, NULL, 0);
    *RedniBrojKupca = 1;

    SemGet(4);
    SemSet(PrazanStol, 1);
    SemSet(KRUH_SUNKA, 0);
    SemSet(SUNKA_SIR, 0);
    SemSet(KRUH_SIR, 0);

    // printf("Pokrećem trgovca i po jednog kupca za svaki sastojak...\n");

    if (fork() == 0) {
        trgovac();
        exit(0);
    }

    // Kupci: 1 = SUNKA, 2 = SIR, 3 = KRUH
    for (int tip = 1; tip <= 3; tip++) {
        pokreni_kupca(tip);
    }

    while (1) {
        pause();
    }

    SemRemove();
    shmdt(RedniBrojKupca);
    shmctl(ShmId, IPC_RMID, NULL);

    return 0;
}
