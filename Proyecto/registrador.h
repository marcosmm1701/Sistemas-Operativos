#ifndef _REGSTRADOR_H
#define _REGSTRADOR_H

#include "miner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <sys/select.h>

#define TIMEOUT_SECONDS 7



int registrador(int pipe_minreg[2], int pipe_regmin[2]);

#endif