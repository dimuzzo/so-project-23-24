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
int N_ATOM = 0;

void signal_handler_sciss();
int energy_freed(int n1,int n2);
int max(int n1,int n2);
void refresh_NATOM();

int main(int argc,char*argv[]){
	
	struct_id = atoi(argv[1]);
    sem_id_sciss = atoi(argv[2]); 
    N_ATOM = atoi(argv[3]);
    my_atoms = shmat(struct_id, NULL, 0);
    
    struct sembuf sops_scissioni = {0, -1, 0};
	semop(sem_id_sciss, &sops_scissioni, 1); 

	refresh_NATOM();
	
	struct sembuf sops_scissioni_up = {0, 1, 0};
	semop(sem_id_sciss, &sops_scissioni_up, 1);
	
	signal(SIGUSR1,signal_handler_sciss); 	
	
	while(1){}
}

void signal_handler_sciss(){
    
    srand(getpid()); 
    
    int i = 0;
    
    if(N_ATOM <= my_atoms->MIN_N_ATOMICO){
        
        struct sembuf sops_scissioni = {0, -1, 0};
		semop(sem_id_sciss, &sops_scissioni, 1); 
		
		my_atoms->scorie += 1;
		/*cerco atomo nel vettore*/
		while(getpid() != my_atoms->atomo[i]){
	        i++;
	    }
	    my_atoms->atomo[i] = -99;
        my_atoms->n_atom[i] = -99;
		
		struct sembuf sops_scissioni_up = {0, 1, 0};
		semop(sem_id_sciss, &sops_scissioni_up, 1);

        kill(getpid(), SIGTERM);
        
    }else{
            
        int delta = rand() % (N_ATOM-1) + 1;
        
        energy_freed(N_ATOM-delta,delta);
        
        char arg_struct[20];
        char arg_semsciss[20];
        char arg_NATOM[20];
        sprintf(arg_struct, "%d", struct_id);
        sprintf(arg_semsciss, "%d", sem_id_sciss);
        sprintf(arg_NATOM, "%d", N_ATOM);
        char *argsAtom[] = {"atomo.out", arg_struct, arg_semsciss, arg_NATOM, NULL};
        
        struct sembuf sops_scissioni = {0, -1, 0};
    	semop(sem_id_sciss, &sops_scissioni, 1);  
    	
    	my_atoms->scissioni++;
    	my_atoms->scissioni_rel++;
    	
    	struct sembuf sops_scissioni_up = {0, 1, 0};
		semop(sem_id_sciss, &sops_scissioni_up, 1);
        
        switch(fork()){
            case -1:  
				kill(my_atoms->master,SIGTERM);
	            exit(EXIT_FAILURE);
	            break;
            case 0:
    			sprintf(argsAtom[3], "%d", delta);
    			execve("./atomo.out", argsAtom, NULL);
	            break;
            default:
            	N_ATOM -= delta;
            	refresh_NATOM();
                break;	
        } 
	
    }
     
}

int energy_freed(int n1,int n2){
    int energy = n1*n2 - max(n1,n2);
    
	struct sembuf sops_scissioni = {0, -1, 0};
	semop(sem_id_sciss, &sops_scissioni, 1);
	
    my_atoms->energy_produced += energy;
    my_atoms->energy_rel_prod += energy;
    
	struct sembuf sops_scissioni_up = {0, 1, 0};
	semop(sem_id_sciss, &sops_scissioni_up, 1);
}

int max(int n1,int n2){
	if(n1 > n2)
		return n1;
	else 
		return n2;
}

void refresh_NATOM(){ /* aggiorno num atomico vettore */ 

    struct sembuf sops_scissioni = {0, -1, 0};
   	semop(sem_id_sciss, &sops_scissioni, 1);  
    
    int i=0;
    pid_t pid = getpid();

    /*cerco l'atomo corrente all'interno del vettore*/
	while(pid != my_atoms->atomo[i] && i<my_atoms->curr_idx){
	    i++;
	}
	
	if(i>=my_atoms->curr_idx){
	    /*caso in cui il pid non c'è in quelli vecchi e dobbiamo salvarlo*/
	    my_atoms->atomo[i] = pid;
        my_atoms->n_atom[i] = N_ATOM;
        my_atoms->curr_idx++;  
	}else{
	    my_atoms->n_atom[i] = N_ATOM;
	}
	
	struct sembuf sops_scissioni_up = {0, 1, 0};
	semop(sem_id_sciss, &sops_scissioni_up, 1);
	
}
