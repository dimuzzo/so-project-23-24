#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "struct.h"

struct atomo *my_atoms;

int STEP;
int N_NUOVI_ATOMI;
int struct_id; /*ID struct*/
int sem_id_sciss; /*ID semaforo*/

void alarm_handler(int signal); /*gestisce creazione nuovi atomi*/

int main(int argc, char *argv[]){
	
	struct_id = atoi(argv[1]);
    sem_id_sciss = atoi(argv[2]);
	
	my_atoms = shmat(struct_id, NULL, 0);
    
    struct sembuf sops_scissioni = {0, -1, 0};
	semop(sem_id_sciss, &sops_scissioni, 1); 
	
    STEP = my_atoms->STEP;
    N_NUOVI_ATOMI = my_atoms->N_NUOVI_ATOMI;
        
	struct sembuf sops_scissioni_up = {0, 1, 0};
	semop(sem_id_sciss, &sops_scissioni_up, 1);
	
	signal(SIGALRM, alarm_handler);
    
	alarm(STEP);
	  
	while(1){} 

}

void alarm_handler(int signal){/* crea ogni step nanosecondi nuovi atomi */

	pid_t pid;
	int i = 0;
	
    int new_NATOM;
    char arg_struct[20];
    char arg_semsciss[20];
    char arg_NATOM[20];
    sprintf(arg_struct, "%d", struct_id);
    sprintf(arg_semsciss, "%d", sem_id_sciss);
    sprintf(arg_NATOM, "%d", 0);
    char *argsAtom[] = {"atomo.out", arg_struct, arg_semsciss, arg_NATOM, NULL};
	
	for(i; i < N_NUOVI_ATOMI; i++){
	   
	   switch(pid = fork()){
		  case -1:
		      fprintf(stderr, "Errore con la fork() \n");
		      kill(getppid(),SIGTERM);
		      exit(EXIT_FAILURE);
		      break;
		  case 0:
		      srand(getpid());
              new_NATOM = rand() % (my_atoms->MAX_N_ATOMICO - my_atoms->MIN_N_ATOMICO + 1) + my_atoms->MIN_N_ATOMICO; /*estraggo numero atomico*/
              sprintf(argsAtom[3], "%d", new_NATOM);
			  execve("./atomo.out", argsAtom, NULL);
              perror("execve");
              exit(EXIT_FAILURE);
		      break;	
		  default:
		      break;	
	   }
	   
   }
   
   alarm(STEP);
   
}

