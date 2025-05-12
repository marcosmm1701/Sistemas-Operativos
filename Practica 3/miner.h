#ifndef MINER_H
#define MINER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>


#include "monitor.h"




#define MESSAGE_KEY 1234 // Clave para la cola de mensajes
#define MAX_ROUNDS 200   // Número máximo de rondas

// Definición de la estructura del mensaje


int main(int argc, char *argv[]);




#endif