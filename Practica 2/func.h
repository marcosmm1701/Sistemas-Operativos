#ifndef FUNC_H
#define FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#define FICHERO "pid.txt"
#define FVOTOS "votos.txt"
#define FPRUEBA "pruebas.txt"
#define SEM_NAME "sem"
#define SEM_CANDI "sem_candidato"
#define SEM_USR1 "sem_u1"
#define SEM_RONDAS "sem_rondas"
#define SEM_SENAL "sem_senal"


#define SHM_SIZE 1024 // para variable global

//#define RAND_MAX 1

typedef enum {
    FALSE,
    TRUE
} BOOL;

BOOL crear_votantes(int num_proc, sem_t *sem, sem_t *sem_candi, sem_t *sem_usr, sem_t *sem_rondas, sem_t *sem_senal, int *v_global);
BOOL enviar_senal_votantes(sem_t *sem_usr, sem_t *sem_rondas);
void ronda();
void terminar_votantes(int num_proc);
void handler(int sig);
void handler_usr(int sig);
BOOL inicio_votacion(sem_t *sem_usr, sem_t *sem);
int votar(int *v_global);
int hijos(int num_proc, sem_t *sem, sem_t *sem_candi, sem_t *sem_usr, sem_t *sem_rondas, int *v_global, int *estatica);



#endif /* FUNC_H */