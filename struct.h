struct atomo{
	pid_t master;
    double energy_produced; /* somma dell'energia liberata dalle scissioni */
    double energy_consumed; /* la consuma il master*/
    double energy_rel_prod;
    double energy_rel_con;
    int atomo[1000];	/*vettore pids */
    int n_atom[1000]; /* vettore numeri atomici */
    int curr_idx; /* ultimo index utilizzato */
    int scorie; /* scorie prodotte dell'atomo */
    int attivazioni; /* attivazioni atomo */
    int attivazioni_rel; /* attivazioni atomo ogni secondo */
    int scissioni; /* numero di scissioni */
    int scissioni_rel;
    int MIN_N_ATOMICO; /* soglia sotto alla quale si generano scorie */
    int MAX_N_ATOMICO;
    int N_NUOVI_ATOMI; /* ALIMENTAZIONE */
    int STEP; /*ogni STEP nanosecondi crea N_NUOVI_ATOMI_ALIMENTAZIONE */
    int SIM_DURATION; /* TIMEOUT */
	int N_ATOM_AT_ONCE; 
    int N_ATOMI_INIT;
	float ENERGY_EXPLODE_THRESHOLD;
};

