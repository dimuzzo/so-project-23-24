# Progetto di Sistemi Operativi: Reazione a Catena

Questo progetto simula una reazione a catena nucleare attraverso l'interazione di diversi processi concorrenti, ognuno con un ruolo specifico all'interno della simulazione.

## Architettura del Sistema

Il sistema è composto da diversi tipi di processi che collaborano per gestire la simulazione.

### 1. Processo Master

Il `master` è il processo principale che orchestra l'intera simulazione.

* **Responsabilità**:
    * Inizializza le strutture dati condivise e le risorse IPC necessarie.
    * Crea i processi figli iniziali: `N_ATOMI_INIT` processi `atomo`, il processo `attivatore` e il processo `alimentazione`.
    * Avvia la simulazione solo dopo che tutti i processi iniziali sono stati creati e inizializzati correttamente.
* **Ciclo di Vita**:
    * Ogni secondo, stampa a schermo lo stato corrente della simulazione, includendo tutte le statistiche richieste.
    * Ogni secondo, preleva una quantità `ENERGY_DEMAND` di energia dal sistema.

### 2. Processo Atomo

I processi `atomo` sono il cuore della reazione a catena.

* **Attributi**: Ogni atomo possiede un numero atomico privato, un valore casuale compreso tra 1 e `N_ATOM_MAX`, assegnato dal suo processo padre al momento della creazione.
* **Scissione (fork)**:
    * Quando un atomo riceve un comando di scissione dal processo `attivatore`, effettua una `fork()`, creando un nuovo processo `atomo`.
    * La somma dei numeri atomici del padre e del nuovo figlio dopo la scissione deve essere uguale al numero atomico del padre prima della scissione.
    * La scissione genera energia, che viene aggiunta alle statistiche globali. La quantità di energia liberata è calcolata secondo la formula: `energy(n1, n2) = n1 * n2 / max(n1, n2)`.
* **Terminazione**: Un atomo con numero atomico inferiore o uguale a `MIN_N_ATOMICO` che riceve un comando di scissione non si scinde, ma termina. Viene conteggiato nelle statistiche come "scoria".

### 3. Processo Attivatore

Il processo `attivatore` ha il compito di stimolare la reazione a catena.

* **Funzionamento**: Ogni `STEP_ATTIVATORE` nanosecondi, seleziona uno o più processi `atomo` attivi e gli comunica di effettuare una scissione.
* **Logica**: Le politiche con cui l'attivatore sceglie quali atomi attivare sono a discrezione dello sviluppatore.
* **Nota**: L'attivatore non crea nuovi atomi direttamente, ma ordina ad atomi esistenti di farlo.

### 4. Processo Alimentazione

Il processo `alimentazione` fornisce nuovo "combustibile" alla simulazione.

* **Funzionamento**: Ogni `STEP_ALIMENTAZIONE` nanosecondi, crea `N_NUOVI_ATOMI` nuovi processi `atomo`, immettendoli nel sistema.

### 5. Processo Inibitore (Versione "Normal")

Presente solo nella versione completa del progetto, l'`inibitore` ha lo scopo di controllare la reazione per evitare che diventi instabile.

* **Meccanismi di Controllo**:
    1.  **Assorbimento Energia**: Assorbe una parte dell'energia prodotta durante le scissioni.
    2.  **Limitazione Scissioni**: Rende probabilistica l'operazione di scissione, potendo decidere se questa debba avvenire o meno, o trasformando uno dei prodotti in scoria.
* **Controllo a Run-time**: L'utente deve poter attivare, fermare e far ripartire l'inibitore più volte durante la simulazione tramite un comando da terminale.
* **Obiettivo**: Se attivo, l'inibitore dovrebbe prevenire le condizioni di terminazione per `explode` e `meltdown`.

## Condizioni di Terminazione

La simulazione termina quando si verifica una delle seguenti condizioni:

* **Timeout**: Viene raggiunta la durata massima della simulazione (`SIM_DURATION`).
* **Explode**: L'energia totale liberata, al netto di quella consumata dal master, supera la soglia `ENERGY_EXPLODE_THRESHOLD`.
* **Blackout**: Il processo master tenta di prelevare una quantità di energia maggiore di quella attualmente disponibile nel sistema.
* **Meltdown**: Una chiamata `fork()` fallisce in uno qualsiasi dei processi, indicando l'esaurimento delle risorse di sistema.

Al termine, il programma deve stampare la causa della terminazione.

## Configurazione

Tutti i parametri della simulazione (es. `N_ATOMI_INIT`, `ENERGY_DEMAND`, etc.) devono essere letti a tempo di esecuzione da un file di configurazione o da variabili d'ambiente. Non è consentito né l'hardcoding dei valori né l'inserimento manuale da terminale dopo l'avvio.

## Requisiti di Implementazione

Il progetto deve rispettare i seguenti requisiti tecnici:

* **Nessuna Attesa Attiva**: Evitare cicli di polling che consumano CPU.
* **IPC**: Utilizzare obbligatoriamente memoria condivisa, semafori e un meccanismo di comunicazione a scelta tra code di messaggi o pipe.
* **Modularità**: Il codice deve essere diviso in moduli. I vari processi devono essere lanciati come eseguibili separati tramite `execve(...)`.
* **Compilazione**: Utilizzare un `Makefile` per la compilazione del progetto.
* **Concorrenza**: Massimizzare il grado di parallelismo tra i processi.
* **Pulizia Risorse**: Deallocare correttamente tutte le risorse IPC allocate al termine della simulazione.
* **Flag di Compilazione**: Compilare con le opzioni `-Wvla -Wextra -Werror`.
* **Parallelismo Reale**: Il programma deve funzionare correttamente su macchine con più processori (fisici o virtuali).

### Prerequisiti

Assicurarsi di avere un ambiente Linux con `gcc` e `make` installati.
