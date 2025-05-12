#include "minero.h"
#include "monitor.h"

void *minar(void *argumento)
{
    int inicio, fin, i;
    ThreadArgs *t_args = argumento;
    inicio = t_args->inicio;
    fin = t_args->fin;

    for (i = inicio; i < fin && *(t_args->finalizado) == FALSE; i++)
    {
        if (pow_hash(i) == t_args->objetivo)
        {
            *(t_args->finalizado) = TRUE;
            *(t_args->resultado) = i;
        }
    }

    pthread_exit(NULL);
}

int minero(int num_rondas, int num_hilos, int objetivo, ThreadArgs *t_args, int pipe_minmon[2], int pipe_monmin[2])
{
    int i, error, resultado;
    BOOL flag;
    pid_t pid_secundario;

    error = pipe(pipe_minmon);
    if (error == -1)
    {
        perror("pipe\n");
        exit(EXIT_FAILURE);
    }

    error = pipe(pipe_monmin);
    if (error == -1)
    {
        perror("pipe\n");
        exit(EXIT_FAILURE);
    }

    pid_secundario = fork();

    /*Rondas*/
    for (i = 0; i < num_rondas; i++)
    {
        if (pid_secundario > 0)
        {
            flag = FALSE;
            resultado = 0;

            close(pipe_minmon[0]);
            close(pipe_monmin[1]);

            error = ronda(num_hilos, objetivo, &flag, &resultado, t_args);
            if (error)
            {
                close(pipe_minmon[1]);
                close(pipe_monmin[0]);
                return EXIT_FAILURE;
            }

            write(pipe_minmon[1], &objetivo, sizeof(int));
            write(pipe_minmon[1], t_args->resultado, sizeof(int));

            objetivo = *(t_args->resultado);

            read(pipe_monmin[0], &error, sizeof(int));

            if (error)
            {
                printf("The solution has been invalidated\n");
                free(t_args);
                close(pipe_minmon[1]);
                close(pipe_monmin[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (pid_secundario == 0)
        {
            error = monitor(pipe_minmon, pipe_monmin);
            if (error)
            {
                free(t_args);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (pid_secundario == 0)
    {
        close(pipe_minmon[0]);
        close(pipe_monmin[1]);
        free(t_args);
        exit(EXIT_SUCCESS);
    }
    else
    {
        close(pipe_minmon[1]);
        close(pipe_monmin[0]);
    }

    return error;
}

int ronda(int num_hilos, int objetivo, BOOL *flag, int *resultado, ThreadArgs *t_args)
{
    int j, range_acc = 0, range, error, z;
    pthread_t h1[num_hilos];

    range = POW_LIMIT / num_hilos;

    for (j = 0; j < num_hilos; j++)
    {
        t_args[j].inicio = range_acc;
        t_args[j].fin = range_acc + range;
        t_args[j].objetivo = objetivo;
        t_args[j].finalizado = flag;
        t_args[j].resultado = resultado;

        range_acc += range + 1;

        // Crear hilo con una copia diferente de la estructura FinalArgs
        error = pthread_create(&h1[j], NULL, minar, &t_args[j]);
        if (error != 0)
        {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }

    for (z = 0; z < num_hilos && z < j; z++)
    {
        error = pthread_join(h1[z], NULL);
        if (error != 0)
        {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}