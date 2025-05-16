#include<stdio.h>
#include<sys/types.h>
#include<math.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<time.h>
#include<stdlib.h>

int  PrazanStol=0; 
int  KRUH_SUNKA=1; /*kruh_sunka*/
int  SUNKA_SIR=2; /*sunka_sir*/
int  KRUH_SIR=3; /*kruh_sir*/

int SemId;

void SemGet(int n){
	SemId = semget(IPC_PRIVATE, n, 0600);
	if(SemId == -1){
		printf("Nema semafora\n");
		exit(1);
	}
}

int SemSetVal(int SemNum, int SemVal){
	return semctl(SemId, SemNum, SETVAL, &SemVal);
}

int SemOp(int SemNum, int SemOp){
	struct sembuf SemBuf;
	SemBuf.sem_num = SemNum;
	SemBuf.sem_op = SemOp;
	SemBuf.sem_flg = 0;
	return semop(SemId, &SemBuf, 1);
}

void SemRemove(void){
	(void) semctl(SemId, 0, IPC_RMID, 0);
}


void pusac(int n){
   switch (n)
   {
   case 1:
   	while(1){
		SemOp(KRUH_SUNKA,-1);
		printf("Kupac 1 uzima sastojke: KRUH, SIR\n");
		SemSetVal(PrazanStol,1);}
      break;
   case 2:
	while(1){
		SemOp(KRUH_SIR, -1);
		printf("Kupac 2 uzima sastojke: KRUH, SUNKA\n");
		SemSetVal(PrazanStol,1);
	}
   break;
   case 3:
	while(1){
		SemOp(SUNKA_SIR, -1);
		printf("Kupac 3 uzima sastojke: SIR, SUNKA.\n");
		SemSetVal(PrazanStol,1);
	}
   break;
   default:
      break;
   }

}		

void trgovac(){
	int sastojak;
	while(1){
		SemOp(PrazanStol, -1);
		srand((unsigned)time(NULL));
		sleep(1);
		sastojak=rand()%3;

   	switch (sastojak)
      {
      case 0:
   		printf("Na stol stavljam KRUH i SUNKA\n");
   		SemOp(KRUH_SIR, 1);      break;
      case 1:
   		printf("Na stol stavljam KRUH i SIR.\n");
   		SemOp(KRUH_SUNKA, 1);
         break;
      case 2:
   	   printf("Na stol stavljam SUNKA i SIR.\n");
   		SemOp(SUNKA_SIR, 1);
         break;
      default:
         break;
      }

      SemOp(PrazanStol, 1);
   }
}
	
		
		
		

int main(void){

   int i;
	
	SemGet(4);
	SemOp(PrazanStol, 1);

	printf("Kupac 1 ima SUNKA.\n");
	printf("Kupac 2 ima SIR.\n");
	printf("Kupac 3 ima KRUH.\n");
	printf("_______________________________\n");
	
	switch(fork()){
		case -1: printf("Ne mogu stvoriti proces!\n");
		case  0: trgovac(1); exit(0);
		default: break;
	}

	switch(fork()){
                case -1: printf("Ne mogu stvoriti proces!\n");
                case  0: pusac(1); exit(0);
                default: break;
        }
	
	switch(fork()){
                case -1: printf("Ne mogu stvoriti proces!\n");
                case  0: pusac(2); exit(0);
                default: break;
        }

	switch(fork()){
                case -1: printf("Ne mogu stvoriti proces!\n");
                case  0: pusac(3); exit(0);
                default: break;
        }
	for(i=0; i<=4; i++){
		wait(NULL);
	}
	SemRemove();

return(0);
}	