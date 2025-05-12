#include "monitor.h"
#include "minero.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

int monitor(int pipe_minmon[2], int pipe_monmin[2])
{
    int objetivo = 0, solucion = 0, status = 0;

    close(pipe_monmin[0]);
    close(pipe_minmon[1]);

    if (getppid() == 1)
        return EXIT_SUCCESS;

    read(pipe_minmon[0], &objetivo, sizeof(int));
    read(pipe_minmon[0], &solucion, sizeof(int));

    if (solucion == 0 && objetivo == 0)
        return EXIT_SUCCESS;

    if (pow_hash(solucion) == objetivo)
    {
        status = EXIT_SUCCESS;
        printf("Solution accepted: %08d --> %08d\n", objetivo, solucion);
    }
    else
    {
        status = EXIT_FAILURE;
        printf("Solution rejected: %08d !-> %08d\n", objetivo, solucion);
    }

    write(pipe_monmin[1], &status, sizeof(int));

    return EXIT_SUCCESS;
}