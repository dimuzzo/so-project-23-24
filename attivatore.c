#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include "struct.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

struct atomo *my_atoms;

int struct_id;
int sem_id_sciss;
int SIM_DURATION;
int N_ATOM_AT_ONCE;

void signal_handler_create(int signal);
void alarm_handler_timeout();

int main(int argc, char *argv[]){

    struct_id = atoi(argv[1]);
    sem_id_sciss = atoi(argv[2]);
    my_atoms = shmat(struct_id, NULL, 0);

    struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);

    SIM_DURATION = my_atoms->SIM_DURATION;
    N_ATOM_AT_ONCE = my_atoms->N_ATOM_AT_ONCE;

    struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);
    
    signal(SIGUSR2,signal_handler_create);
    signal(SIGALRM,alarm_handler_timeout);     

    alarm(SIM_DURATION);

    while(1){}

}

void signal_handler_create(int signal){
	int received_value;

    /*Lettura del valore intero dalla pipe*/
    ssize_t bytes_read = read(STDIN_FILENO, &received_value, sizeof(received_value));
    if (bytes_read == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
	
	struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);
    
    if(my_atoms->atomo[received_value] != 0){
    	my_atoms->attivazioni++; 
	    my_atoms->attivazioni_rel++;
        kill(my_atoms->atomo[received_value],SIGUSR1);
        
    }
	
	struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);
}

void alarm_handler_timeout(){
	kill(getppid(),SIGUSR2);
}

