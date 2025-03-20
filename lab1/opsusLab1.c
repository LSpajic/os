#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "stack.h"

int nije_kraj = 1;
int K_Z = 0;
int T_P = 0;
Stack stack;

void obradi_sigint(int sig);
void odblokiraj(void);
void emptyCounter(int signalPriority);
int returnHighestPriority(int K_Z);

int main()
{
    initialize(&stack);

    struct sigaction act;
    // Initialize sigaction structure
    act.sa_handler = obradi_sigint;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);
    printf("Program s PID=%ld krenuo s radom\n", (long)getpid());

    int i = 1;
    nije_kraj = 20;

    emptyCounter(0);
    printf("Program zavrsio s radom\n");
    return 0;
}

void emptyCounter(int signalPriority)
{
    odblokiraj();
    int counter = 0;
    int i = 0;
    while (i < 10)
    {
        printf("Signal razine: %d, iteracija: %d\n", signalPriority, i);
        sleep(1);
        i++;
    }
    printf("Zavrsio signal razine: %d\n", signalPriority);
    printf("54321\n");
    int H_P = 1 << (returnHighestPriority(K_Z) - 1);
    int a = peek(&stack);
    if (H_P > a)
    {
        T_P = H_P;
        K_Z &= ~H_P;
        printf("K_Z: ");
        binprintf(K_Z);
        printf(", TP: ");
        binprintf(T_P);
        printf("\n");

        emptyCounter(returnHighestPriority(T_P));
    }
    else if (!isEmpty(&stack))
    {
        T_P = pop(&stack);
        printf("K_Z: ");
        binprintf(K_Z);
        printf(", TP: ");
        binprintf(T_P);
    }
    printStack(&stack);
}
void odblokiraj(void)
{
    sigset_t signali;
    sigemptyset(&signali);
    sigaddset(&signali, SIGINT);
    pthread_sigmask(SIG_UNBLOCK, &signali, NULL);
}
int returnHighestPriority(int a)
{
    int highestBit = 0;
    int bitPosition = 0;
    if (a == 0)
    {
        return 0;
    }
    while (a > 0)
    {
        if (a & 1)
        {
            highestBit = bitPosition;
        }
        a >>= 1;
        bitPosition++;
    }

    return highestBit + 1;
}

void obradi_sigint(int sig)
{
    int prioritet;
    printf("\nUnesi razinu prioriteta[1-5]: ");
    scanf("%d", &prioritet);
    if (prioritet == 6)
    {
        printf("Gasim program\n");
        exit(0);
    }
    int R_P = 1 << (prioritet - 1);
    K_Z |= R_P;
    int najveci_prioritet = returnHighestPriority(K_Z);
    int H_P = 1 << (najveci_prioritet - 1);
    if (H_P > T_P)
    {
        printf("Signal vise razine, stavljam TP na stog\n");
        push(&stack, T_P);
        T_P = H_P;
        K_Z &= ~H_P;
        emptyCounter(najveci_prioritet);
    }
    else
    {
        printf("Signal manje razine, Spremam u K_Z\n");
    }
}
