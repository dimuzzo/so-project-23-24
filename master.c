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

pid_t attivatore;
pid_t alimentazione;

struct atomo *my_atoms;

int N_ATOMI_INIT, NUM_CHILDREN;
float ENERGY_EXPLODE_THRESHOLD;
int sem_id_sciss;
int pipefds[2];

void print_daily();
void energy_wd();
void alarm_handler();
void init();
void terminazione_timeout();
void terminazione_meltdown();
void terminazione_explode();
void terminazione_blackout();
void clean_atoms();
void extract_number();

int main(int argc, char* argv[]) {

    int i=0;
    pid_t child_pid, pid;
    int status;
    
    /*creazione della pipe*/
    if (pipe(pipefds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    /* Ottieni l'ID della memoria condivisa */
    int struct_id = shmget(IPC_PRIVATE, sizeof(struct atomo), 0600);
    if (struct_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    /* Associa la memoria condivisa al puntatore my_atoms */
    my_atoms = shmat(struct_id, NULL, 0);
    if (my_atoms == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    /* Inizializza la memoria condivisa */
    init();

    /* Rimuovi la memoria condivisa quando non e' piu' necessaria */
    shmctl(struct_id, IPC_RMID, NULL);

    /* Inizializza il semaforo per la variabile scissioni */
    sem_id_sciss = semget(IPC_PRIVATE, 1, 0600);
    if (sem_id_sciss == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    semctl(sem_id_sciss, 0, SETVAL, 1);
	
	/* alarm handlers */
	signal(SIGALRM, alarm_handler);
    signal(SIGUSR2, terminazione_timeout);
	signal(SIGTERM, terminazione_meltdown);
	
	printf("\nINIZIO SIMULAZIONE\n");
	
    /*Creazione dei processi figli*/
    int new_NATOM;
    char arg_struct[20];
    char arg_sem[20];
    char arg_semsciss[20];
    char arg_NATOM[20];
    sprintf(arg_struct, "%d", struct_id);
    sprintf(arg_sem, "%d", sem_id_sciss);
    sprintf(arg_NATOM, "%d", 0);
    char *args[] = {".out", arg_struct, arg_sem, NULL};
    char *argsAtom[] = {"atomo.out", arg_struct, arg_sem, arg_NATOM, NULL};
    
    for (i = 0; i < 2+N_ATOMI_INIT; i++) {
    
        switch (pid = fork()) {
            case -1:
                fprintf(stderr, "Errore con la fork() \n");
                terminazione_meltdown();
                exit(EXIT_FAILURE);
                break;
            case 0: 
               
                switch (i) {
                    
                    case 0:
                        args[0] = "alimentazione.out";
                        execve("./alimentazione.out", args, NULL);
                        perror("execve");
                        exit(EXIT_FAILURE);
                        break;
                    case 1:
                        dup2(pipefds[0], STDIN_FILENO); /*Redireziona l'input standard del processo figlio alla pipe*/
                        args[0] = "attivatore.out";
                        execve("./attivatore.out", args, NULL);
                        perror("execve");
                        exit(EXIT_FAILURE);
                        break;
                    default:
                        srand(getpid());
                        new_NATOM = rand() % (my_atoms->MAX_N_ATOMICO - my_atoms->MIN_N_ATOMICO + 1) + my_atoms->MIN_N_ATOMICO;
                        sprintf(argsAtom[3], "%d", new_NATOM);
                        execve("./atomo.out", argsAtom, NULL);
                        perror("execve");
                        exit(EXIT_FAILURE);
                        break;
                }
                
                break;
            default:
                switch (i) {
                    case 0:
                        alimentazione = pid;
                        break;
                    case 1:
                        attivatore = pid;
                        break;
            		default:break;
                }
                break;
        }
        
    }
	NUM_CHILDREN = 4; /* 3 (attivatore,alimentazione,master) + 1 atomo */
    /* Incrementa i semafori per avviare i processi figli*/
    for (i = 0; i < NUM_CHILDREN; i++) {
        struct sembuf sops = {0, 1, 0};
        semop(sem_id_sciss, &sops, 1);
    }

    /* Imposta l'allarme per la stampa periodica dei risultati*/
    alarm(1);

    /* Gestione della terminazione dei processi figli*/
    while ((child_pid = wait(&status)) != -1) {
        pause();
    }
	
    /* Rimozione dei semafori*/
    semctl(sem_id_sciss, 0, IPC_RMID);
    
    close(pipefds[0]); /*Chiusura del lato di lettura della pipe nel padre*/

    return 0;
}

void alarm_handler() {
	struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);
    
	int energia_disp = my_atoms->energy_produced;
	int energia_cons = my_atoms->energy_consumed;
	
	struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);
    
    /*energia_disp = energia dispersa*/
    
	if(energia_disp - energia_cons <= ENERGY_EXPLODE_THRESHOLD && energia_disp - energia_cons >= 0){
		clean_atoms();
		print_daily();
		extract_number();
		kill(attivatore,SIGUSR2); /*ordino la scissione agli atomi*/
		energy_wd();
		alarm(1);
	} else{
		if(energia_disp - energia_cons > ENERGY_EXPLODE_THRESHOLD){
			terminazione_explode();
		} else if(energia_disp - energia_cons < 0) {
			terminazione_blackout();
		}
	}
}

void terminazione_timeout() {
	int i=0;
	
	struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);
    
	printf("\n\n!-!-!-!-\tTIMEOUT\t!-!-!-!-!\n");
	printf("Termino tutti i programmi\n");
	for(i = 0;i < my_atoms->curr_idx;i++){
		kill(my_atoms->atomo[i],SIGINT);/*termino tutti i figli*/
	}
	kill(attivatore,SIGINT);/*termino attivatore*/
	kill(alimentazione,SIGINT);/*termino alimentazione*/
	semctl(sem_id_sciss, 0, IPC_RMID);
	
	struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);
	exit(EXIT_SUCCESS);
}

void terminazione_meltdown(){
    int i = 0;
	printf("\n\n!-!-!-!-\tMELTDOWN\t!-!-!-!-!\n");
	printf("Termino tutti i programmi\n");
	for(i;i < my_atoms->curr_idx;i++){
		kill(my_atoms->atomo[i],SIGINT);/*termino tutti i figli*/
	}
	kill(attivatore,SIGINT);/*termino attivatore*/
	kill(alimentazione,SIGINT);/*termino alimentazione*/
	exit(EXIT_SUCCESS);
}

void terminazione_explode(){
	int i=0;
	struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);
    
    printf("\n\n!-!-!-!-\tEXPLODE\t!-!-!-!-!\n");
	printf("Explode: energia libera maggiore rispetto a soglia indicata in ENERGY_EXPLODE_TRESHOLD.\n");
    printf("Termino tutti i programmi\n");
	for(i;i < my_atoms->curr_idx;i++){
		kill(my_atoms->atomo[i],SIGINT);/*termino tutti i figli*/
	}
	kill(attivatore,SIGINT);/*termino attivatore*/
	kill(alimentazione,SIGINT);/*termino alimentazione*/
	semctl(sem_id_sciss, 0, IPC_RMID);
	
	struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);
	exit(EXIT_FAILURE);
}

void terminazione_blackout(){
	int i=0;
    struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);
    
	printf("\n\n!-!-!-!-\tBLACKOUT\t!-!-!-!-!\n");
	printf("Blackout: prelievo energia maggiore rispetto a quella disponibile.\n");
	printf("Termino tutti i programmi\n");
	for(i;i < my_atoms->curr_idx;i++){
		kill(my_atoms->atomo[i],SIGINT);/*termino tutti i figli*/
	}
	kill(attivatore,SIGINT);/*termino attivatore*/
	kill(alimentazione,SIGINT);/*termino alimentazione*/
	semctl(sem_id_sciss, 0, IPC_RMID);
	
	struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);
	exit(EXIT_FAILURE);
}

void print_daily() {
	struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);
    
    printf("\n\n!-!-!-!-\tPRINT DAILY\t!-!-!-!-!\n");
    printf("1. Attivazioni totali: %d, relative: %d\n", my_atoms->attivazioni, my_atoms->attivazioni_rel);
    printf("2. Scissioni totali: %d, relative: %d\n", my_atoms->scissioni, my_atoms->scissioni_rel);
    printf("3. Energia prodotta totale: %f, relativa: %f\n", my_atoms->energy_produced, my_atoms->energy_rel_prod);
    printf("4. Energia consumata totale: %f, relativa: %f\n", my_atoms->energy_consumed, my_atoms->energy_rel_con);
    printf("5. Scorie: %d\n", my_atoms->scorie); 
    printf("\n");
	
	struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);
}

void energy_wd(){
	int energy_consume = 0;
	
	struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);
    
    my_atoms->attivazioni_rel = 0;
    my_atoms->scissioni_rel = 0;
    my_atoms->energy_rel_prod = 0;
    my_atoms->energy_rel_con = 0;
	
	srand(getpid());
	energy_consume = rand() % (100) + 1;
		
	my_atoms->energy_consumed += energy_consume;
	my_atoms->energy_rel_con = energy_consume;
	
	struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);
}

void clean_atoms(){
    /*cancella i pid -99 (scorie) dall'array e dagli N_ATOM;*/
	struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);
	
	int new_atomi[1000];
	int new_n_atomi[1000];
	int j = 0;
	int i = 0;
	int z = 0;
	int contatore = 0;
	for(i = 0;i<1000;i++){
		if(my_atoms->atomo[i] != -99){
			new_atomi[j] = my_atoms->atomo[i];
			j++;
		}
		else{
			contatore ++;
		}
	}
	
	j=0;
	
	for(i=0;i<1000;i++){
		if(my_atoms->n_atom[i] != -99){
			new_n_atomi[j] = my_atoms->n_atom[i];
			j++;
		}
	}
	
	my_atoms->curr_idx -= contatore;
	
	for(z = 0; z < my_atoms->curr_idx;z++){
			my_atoms->atomo[z] = new_atomi[z];
			my_atoms->n_atom[z] = new_n_atomi[z];
	}
	struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);

}

void init() {
    /* Inizializza la struttura atomo nella memoria condivisa*/
    /* nessuna necessita' di sincronizzare gli access (unico processo in esecuzione e' master)*/
    FILE *oc = fopen("config.txt", "r");
    char buffer[128];
    char c;
    
    if(oc == NULL){
       printf("Errore nell'apertura del file. \n");
    }
    
    else{
       printf("Contenuto della configurazione: \n");
       while(fscanf(oc, "%s %c", buffer, &c) != EOF){
       	   if(strcmp(buffer, "MIN_N_ATOMICO") == 0){
              fscanf(oc, "%d %c", &my_atoms->MIN_N_ATOMICO, &c);
           }
           if(strcmp(buffer, "MAX_N_ATOMICO") == 0){
              fscanf(oc, "%d %c", &my_atoms->MAX_N_ATOMICO, &c);
           }
           if(strcmp(buffer, "STEP") == 0){
              fscanf(oc, "%d %c", &my_atoms->STEP, &c);
           }
           if(strcmp(buffer, "SIM_DURATION") == 0){
              fscanf(oc, "%d %c", &my_atoms->SIM_DURATION, &c);
           }
           if(strcmp(buffer, "N_ATOM_AT_ONCE") == 0){
              fscanf(oc, "%d %c", &my_atoms->N_ATOM_AT_ONCE, &c);
           }
           if(strcmp(buffer, "N_ATOMI_INIT") == 0){
              fscanf(oc, "%d %c", &my_atoms->N_ATOMI_INIT, &c);
           }
           if(strcmp(buffer, "ENERGY_EXPLODE_THRESHOLD") == 0){
              fscanf(oc, "%f %c", &my_atoms->ENERGY_EXPLODE_THRESHOLD, &c);
           } 
       }
    }
    fclose(oc);
    printf("MIN_N_ATOMICO = %d; \n", my_atoms->MIN_N_ATOMICO);
    printf("MAX_N_ATOMICO = %d; \n", my_atoms->MAX_N_ATOMICO);
    printf("STEP = %d; \n", my_atoms->STEP);
    printf("SIM_DURATION = %d; \n", my_atoms->SIM_DURATION);
    printf("N_ATOM_AT_ONCE = %d; \n", my_atoms->N_ATOM_AT_ONCE);
    printf("N_ATOMI_INIT = %d; \n", my_atoms->N_ATOMI_INIT);
    printf("ENERGY_EXPLODE_THRESHOLD = %f; \n", my_atoms->ENERGY_EXPLODE_THRESHOLD);
    
    my_atoms->master = getpid();
    my_atoms->scissioni = 0;
    my_atoms->attivazioni = 0;
    my_atoms->attivazioni_rel = 0;
    my_atoms->scissioni_rel = 0;
    my_atoms->energy_produced = 100;
    my_atoms->energy_consumed = 0;
    my_atoms->energy_rel_prod = 0;
    my_atoms->energy_rel_con = 0;
    my_atoms->scorie = 0;
    
    N_ATOMI_INIT = my_atoms->N_ATOMI_INIT;
    ENERGY_EXPLODE_THRESHOLD = my_atoms->ENERGY_EXPLODE_THRESHOLD;
    
}

void extract_number() {
    /*estrae l'atomo da scidere e lo comunico ad attivatore*/
    struct sembuf sops_scissioni = {0, -1, 0};
    semop(sem_id_sciss, &sops_scissioni, 1);
    
    int value_to_send;    
    srand(getpid());
    value_to_send = rand() % (my_atoms->curr_idx-1) + 1;
    write(pipefds[1], &value_to_send, sizeof(value_to_send)); /* Scrittura del valore intero sulla pipe*/
    
    struct sembuf sops_scissioni_up = {0, 1, 0};
    semop(sem_id_sciss, &sops_scissioni_up, 1);
}

