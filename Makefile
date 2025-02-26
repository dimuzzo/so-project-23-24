CC=gcc
CFLAGS=-g -O0 -std=c89 -Wvla -Wextra -Werror
exe=./master.out
# Elenco dei nomi degli eseguibili finali
EXECUTABLES=master.out atomo.out attivatore.out alimentazione.out

all: $(EXECUTABLES)

master.out: master.c struct.h
	$(CC) $(CFLAGS) $< -o $@

atomo.out: atomo.c struct.h
	$(CC) $(CFLAGS) $< -o $@

attivatore.out: attivatore.c struct.h
	$(CC) $(CFLAGS) $< -o $@

alimentazione.out: alimentazione.c struct.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLES)
run: $(EXECUTABLES)
	$(exe)

