#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    int broj_studenata_u_sobi;
    int partibrejker_u_sobi;
    int broj_studenata_u_sustavu;
    
    pthread_cond_t student_moze_uci;
    pthread_cond_t partibrejker_moze_uci;
    pthread_cond_t partibrejker_moze_izaci;
    
    pthread_mutex_t monitor;
} Zabava;

Zabava zabava;

void inicijaliziraj_monitor(int N) {
    zabava.broj_studenata_u_sobi = 0;
    zabava.partibrejker_u_sobi = 0;
    zabava.broj_studenata_u_sustavu = N;
    
    pthread_mutex_init(&zabava.monitor, NULL);
    pthread_cond_init(&zabava.student_moze_uci, NULL);
    pthread_cond_init(&zabava.partibrejker_moze_uci, NULL);
    pthread_cond_init(&zabava.partibrejker_moze_izaci, NULL);
}

void student_ulazi(int id) {
    pthread_mutex_lock(&zabava.monitor);
    
    while (zabava.partibrejker_u_sobi) {
        pthread_cond_wait(&zabava.student_moze_uci, &zabava.monitor);
    }
    
    zabava.broj_studenata_u_sobi++;
    printf("Student %d je usao u sobu (ukupno studenata: %d)\n", id, zabava.broj_studenata_u_sobi);
    
    if (zabava.broj_studenata_u_sobi >= 3) {
        pthread_cond_signal(&zabava.partibrejker_moze_uci);
    }
    
    pthread_mutex_unlock(&zabava.monitor);
}

void student_izlazi(int id) {
    pthread_mutex_lock(&zabava.monitor);
    
    zabava.broj_studenata_u_sobi--;
    printf("Student %d je izasao iz sobe (ukupno studenata: %d)\n", id, zabava.broj_studenata_u_sobi);
    
    if (zabava.partibrejker_u_sobi && zabava.broj_studenata_u_sobi == 0) {
        pthread_cond_signal(&zabava.partibrejker_moze_izaci);
    }
    
    pthread_mutex_unlock(&zabava.monitor);
}

void partibrejker_ulazi() {
    pthread_mutex_lock(&zabava.monitor);
    
    while (zabava.broj_studenata_u_sobi < 3) {
        pthread_cond_wait(&zabava.partibrejker_moze_uci, &zabava.monitor);
    }
    
    zabava.partibrejker_u_sobi = 1;
    printf("Partibrejker je usao u sobu (broj studenata: %d)\n", zabava.broj_studenata_u_sobi);
    
    pthread_mutex_unlock(&zabava.monitor);
}

void partibrejker_izlazi() {
    pthread_mutex_lock(&zabava.monitor);
    
    while (zabava.broj_studenata_u_sobi > 0) {
        pthread_cond_wait(&zabava.partibrejker_moze_izaci, &zabava.monitor);
    }
    
    zabava.partibrejker_u_sobi = 0;
    printf("Partibrejker je izasao iz sobe\n");
    
    pthread_cond_broadcast(&zabava.student_moze_uci);
    
    pthread_mutex_unlock(&zabava.monitor);
}

void* student(void* arg) {
    int id = *((int*)arg);
    
    int vrijeme_spavanja = (rand() % 401) + 100;
    usleep(vrijeme_spavanja * 1000);
    
    for (int i = 0; i < 3; i++) {
        student_ulazi(id);
        
        int vrijeme_zabave = (rand() % 1001) + 1000;
        usleep(vrijeme_zabave * 1000);
        
        student_izlazi(id);
        
        int vrijeme_odmora = (rand() % 1001) + 1000;
        usleep(vrijeme_odmora * 1000);
    }
    
    pthread_mutex_lock(&zabava.monitor);
    zabava.broj_studenata_u_sustavu--;
    pthread_mutex_unlock(&zabava.monitor);
    
    pthread_exit(NULL);
}

void* partibrejker(void* arg) {
    while (1) {
        pthread_mutex_lock(&zabava.monitor);
        int ima_studenata = zabava.broj_studenata_u_sustavu > 0;
        pthread_mutex_unlock(&zabava.monitor);
        
        if (!ima_studenata) {
            break;
        }
        
        int vrijeme_spavanja = (rand() % 901) + 100;
        usleep(vrijeme_spavanja * 1000);
        
        partibrejker_ulazi();
        
        partibrejker_izlazi();
    }
    
    printf("Partibrejker zavrsava rad jer nema vise studenata u sustavu\n");
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Upotreba: %s <broj_studenata>\n", argv[0]);
        return 1;
    }
    
    int N = atoi(argv[1]);
    if (N <= 3) {
        printf("Broj studenata mora biti veći od 3!\n");
        return 1;
    }
    
    srand(time(NULL));
    
    inicijaliziraj_monitor(N);
    
    pthread_t* student_dretve = malloc(N * sizeof(pthread_t));
    int* student_id = malloc(N * sizeof(int));
    pthread_t partibrejker_dretva;
    
    pthread_create(&partibrejker_dretva, NULL, partibrejker, NULL);
    
    for (int i = 0; i < N; i++) {
        student_id[i] = i + 1;
        pthread_create(&student_dretve[i], NULL, student, &student_id[i]);
    }
    
    for (int i = 0; i < N; i++) {
        pthread_join(student_dretve[i], NULL);
    }
    
    pthread_join(partibrejker_dretva, NULL);
    
    pthread_mutex_destroy(&zabava.monitor);
    pthread_cond_destroy(&zabava.student_moze_uci);
    pthread_cond_destroy(&zabava.partibrejker_moze_uci);
    pthread_cond_destroy(&zabava.partibrejker_moze_izaci);
    
    free(student_dretve);
    free(student_id);
    
    printf("Program uspješno završio\n");
    return 0;
}