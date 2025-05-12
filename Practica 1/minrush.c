#include "minero.h"
#include "pow.h"
#include "monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Error en el numero de argumentos\n");
        return EXIT_FAILURE;
    }

    int num_rondas, num_hilos, objetivo, error = 0;
    pid_t pid_principal;

    objetivo = atoi(argv[1]);
    num_rondas = atoi(argv[2]);
    num_hilos = atoi(argv[3]);

    pid_principal = fork();

    if (pid_principal == 0)
    {
        int pipe_minmon[2], pipe_monmin[2];

        ThreadArgs *t_args;

        t_args = (ThreadArgs *)malloc(num_hilos * sizeof(ThreadArgs));
        if (!t_args)
        {
            fprintf(stderr, "Error reservando memoria para final_args\n");
            exit(EXIT_FAILURE);
        }

        error = minero(num_rondas, num_hilos, objetivo, t_args, pipe_minmon, pipe_monmin);
        if (error)
        {
            free(t_args);
            exit(EXIT_FAILURE);
        }

        free(t_args);

        exit(EXIT_SUCCESS);
    }

    waitpid(pid_principal, &error, 0);

    if (WIFEXITED(error))
    {
        printf("Monitor exited with status %d\n", EXIT_SUCCESS);
        printf("Miner exited with status %d\n", WEXITSTATUS(error));
    }
    else
    {
        printf("Monitor exited unexpectedly\n");
        printf("Miner exited unexpectedly\n");
    }

    return error;
}