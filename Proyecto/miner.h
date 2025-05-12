#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <signal.h>
#include "pow.h"
#include "registrador.h"

#ifndef MINER_H
#define MINER_H

#define MAX_STR 128
#define BUFFER_SIZE 6
#define MAX_MINEROS 100
#define MAX_VOTOS 100
#define SHM_NAME "/blockchain"
#define MQ_NAME "/cola_mensajes"


typedef enum
{
    FALSE,
    TRUE
} BOOL;

typedef struct ThreadArgs
{
    int inicio;
    int fin;
    int objetivo;
    int *resultado;
    BOOL *finalizado;
} ThreadArgs;



// Estructura para representar un bloque
typedef struct
{
    int id;
    int objetivo;
    int solucion;
    pid_t pid_minero_ganador;
    int num_votos_totales;
    int num_votos_positivos;
    struct{
        pid_t pid_minero;
        int monedas;
    }cartera_min_act[MAX_MINEROS];
    int contador;

} Bloque;

typedef struct
{
    sem_t sem_empty, sem_fill, sem_mutex;
    struct{
        pid_t pid_minero;
        int monedas;
        int votos;
    }datos_minero[MAX_MINEROS];
    int cont;
    Bloque bloque_act;
    Bloque ult_Bloque_res;
    int num_minando;
} Sistema;

void *minar(void *estructura);
int minero(int num_rondas, int num_hilos, int objetivo, ThreadArgs *t_args, int pipe_minmon[2], int pipe_monmin[2]);
int ronda(int num_hilos, int objetivo, BOOL *flag, int *resultado, ThreadArgs *t_args);
int proceso_de_minado(Sistema *buffer, int num_hilos, int objetivo);
void enviar_senal_mineros(Sistema *buffer);
void handler_sigusr1(int sig);
void handler_sigusr2(int sig);
void handler_sigPerdida(int sig);
void handler_SIGINT(int sig);

#endif