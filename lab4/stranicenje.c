#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

#define N 2 // N procesa
#define M 1 // M okvira u RAMu
#define PAGE_SIZE 1 // velicina stranice u bajtovima
#define NUM_PAGES 1 // broj stranica po procesu
#define LOGICAL_ADDRESS_SPACE (PAGE_SIZE * NUM_PAGES)

sem_t *sem;

unsigned char (*disk)[PAGE_SIZE * NUM_PAGES];
unsigned char (*okvir)[PAGE_SIZE];

typedef struct {
    int process_id;
    int page_number;
    int recently_used;
    int valid;
} Frame;

Frame *frame_metadata;
Frame (*disk_metadata)[NUM_PAGES];
Frame empty_frame = {-1, -1, 0, 0};

int *clock_hand;

typedef struct {
    int frame;
    int valid;
} PageTableEntry;

PageTableEntry (*page_table)[NUM_PAGES];

void print_clock() {
    printf("\n[Clock Hand Position: %d]\n", *clock_hand);
    for (int i = 0; i < M; i++) {
        Frame f = frame_metadata[i];
        printf(" %d ", f.recently_used);
    }
    printf("\n");
}

void print_frame_table() {
    printf("\n[Frame Table State]\n");
    printf(" Frame | Proc | Page | RefBit \n");
    for (int i = 0; i < M; i++) {
        Frame f = frame_metadata[i];
        printf("  %2d   |  %2d  |  %2d  |   %d   \n", 
            i, f.process_id, f.page_number, f.recently_used);
    }
}


void swap_page(int pid, int page_number, int frame_number) {
    if (frame_metadata[frame_number].valid) {
        int old_pid = frame_metadata[frame_number].process_id;
        int old_page_number = frame_metadata[frame_number].page_number;
        for (int i = 0; i < PAGE_SIZE; i++) {
            disk[old_pid][old_page_number * PAGE_SIZE + i] = okvir[frame_number][i];
        }
        frame_metadata[frame_number].valid = 0;
    }

    frame_metadata[frame_number].process_id = pid;
    frame_metadata[frame_number].page_number = page_number;
    frame_metadata[frame_number].recently_used = 1;
    frame_metadata[frame_number].valid = 1;

    for (int i = 0; i < PAGE_SIZE; i++) {
        okvir[frame_number][i] = disk[pid][page_number * PAGE_SIZE + i];
    }

    page_table[pid][page_number].frame = frame_number;
    page_table[pid][page_number].valid = 1;
}

int dohvati_fizicku_adresu(int p, int x) {
    sem_wait(sem);

    print_frame_table();
    unsigned int page_number = x / PAGE_SIZE;
    unsigned int offset = x % PAGE_SIZE;

    for (int i = 0; i < M; i++) {
        if (frame_metadata[i].process_id == p && frame_metadata[i].page_number == page_number) {
            frame_metadata[i].recently_used = 1;
            printf("[Process %d] stranica %d u okviru %d\n", p, page_number, i);
            sem_post(sem);
            return i * PAGE_SIZE + offset;
        }
    }

    print_clock();
    printf("[Process %d] Stranica %d nije u memoriji, mijenjam okvir...\n", p, page_number);
    int frame_to_replace = -1;
    while (1) {
        if (frame_metadata[*clock_hand].page_number == -1) {
            frame_to_replace = *clock_hand;
            printf("[Process %d] nadjen prazan okvir %d\n", p, frame_to_replace);
            break;
        } else if (frame_metadata[*clock_hand].recently_used == 0) {
            frame_to_replace = *clock_hand;
            *clock_hand = (*clock_hand + 1) % M;
            printf("[Process %d] mijenjam okvir %d sa stranicom %d\n", p, *clock_hand, page_number);
            break;
        } else {
            frame_metadata[*clock_hand].recently_used = 0;
            *clock_hand = (*clock_hand + 1) % M;
        }
        print_clock();
    }

    swap_page(p, page_number, frame_to_replace);
    printf("[Process %d] zamijenio okvir %d sa stranicom %d\n", p, frame_to_replace, page_number);
    print_frame_table();

    sem_post(sem);
    return frame_to_replace * PAGE_SIZE + offset;
}

int dohvati_sadrzaj(int p, int x) {
    unsigned y = dohvati_fizicku_adresu(p, x);
    printf("[Process %d] Read %d Fadr: 0x%x\n", p, okvir[y / PAGE_SIZE][y % PAGE_SIZE], y);
    return okvir[y / PAGE_SIZE][y % PAGE_SIZE];
}

void zapisi_vrijednost(int p, int x, int v) {
    unsigned y = dohvati_fizicku_adresu(p, x);
    okvir[y / PAGE_SIZE][y % PAGE_SIZE] = v;
    printf("[Process %d] Write %d Fadr: 0x%x\n", p, v, y);
}

void simuliraj_procese() {
    for (int i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            srand(time(NULL) ^ (getpid()<<16));
            while (1) {
                int x = rand() % LOGICAL_ADDRESS_SPACE;
                x = x & 0x3FE;
                printf("[Proces %d] Generirana adresa: 0x%x\n", i, x);
                unsigned int v = dohvati_sadrzaj(i, x);
                v = (v + 1) % 256;
                zapisi_vrijednost(i, x, v);
                printf("[Proces %d] LAdr: 0x%x, Vrijednost: %d\n", i, x, v);
                usleep((rand() % 900 + 2000) * 1000);
            }
            exit(0);
        }
    }

    for (int i = 0; i < N; i++) {
        wait(NULL);
    }
}

int main() {
    disk = mmap(NULL, sizeof(unsigned char) * N * PAGE_SIZE * NUM_PAGES,
                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    okvir = mmap(NULL, sizeof(unsigned char) * M * PAGE_SIZE,
                 PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    frame_metadata = mmap(NULL, sizeof(Frame) * M,
                          PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    disk_metadata = mmap(NULL, sizeof(Frame) * N * NUM_PAGES,
                         PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    page_table = mmap(NULL, sizeof(PageTableEntry) * N * NUM_PAGES,
                      PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    clock_hand = mmap(NULL, sizeof(int),
                      PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    sem = mmap(NULL, sizeof(sem_t),
               PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    for (int i = 0; i < M; i++) {
        frame_metadata[i] = empty_frame;
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < NUM_PAGES; j++) {
            disk_metadata[i][j] = empty_frame;
            page_table[i][j].valid = 0;
        }
    }

    *clock_hand = 0;

    sem_init(sem, 1, 1);

    srand(time(NULL));

    simuliraj_procese();

    return 0;
}
