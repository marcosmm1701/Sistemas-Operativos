#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "pow.h"

#ifndef FUNC_H
#define FUNC_H

#define MAX_STR 128

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

void *minar(void *estructura);
int minero(int num_rondas, int num_hilos, int objetivo, ThreadArgs *t_args, int pipe_minmon[2], int pipe_monmin[2]);
int ronda(int num_hilos, int objetivo, BOOL *flag, int *resultado, ThreadArgs *t_args);

#endif