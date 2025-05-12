#ifndef MONITOR_H
#define MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <fcntl.h>
#include <math.h>
#include "pow.h"


#define SHM_NAME "/blockchain_monitor"
#define MQ_NAME "/cola_mensajes"
#define BUFFER_SIZE 6

// Estructura para representar un bloque
typedef struct
{
  long int objetivo;
  long int solucion;
  int es_correcto; // Bandera que indica si el bloque es correcto o no
} Bloque;

typedef struct
{
  sem_t sem_empty, sem_fill, sem_mutex;
  Bloque bloques[BUFFER_SIZE];
} DatosBloques;

struct message
{
    int objetivo;  // Objetivo de la prueba de trabajo
    int resultado; // Solución encontrada
};

// Declaración de funciones
void comprobador(int lag, DatosBloques *buffer);
void monitor(int lag, DatosBloques *buffer);
int main(int argc, char *argv[]);

#endif // MONITOR_H