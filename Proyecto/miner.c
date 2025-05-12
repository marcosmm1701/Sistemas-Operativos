#include "miner.h"

volatile BOOL bandera_u2 = FALSE;     // Variable global para la señal SIGUSR2
volatile BOOL senal_perdida1 = FALSE; //
volatile BOOL senal_perdida2 = FALSE; //
volatile Sistema *mem;
volatile int fd_shm;
volatile BOOL term = FALSE;

int main(int argc, char *argv[])
{
    int flag = 0;
    Sistema *buffer;
    int lag = atoi(argv[1]); // Retraso
    int num_hilos = atoi(argv[2]);
    int objetivo = 0;
    struct sigaction act_usr, act, alarma;
    int error;
    int pid_secundario;
    int pipe_minreg[2];
    int pipe_regmin[2];

    // Si es el primero (Minero), creamos el segmento de memoria compartida
    fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_shm == -1)
    {
        printf("0\n");
        // Si la memoria compartida ya está creada (segundos Mineroos), la abrimos
        fd_shm = shm_open(SHM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
        if (fd_shm == -1)
        {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
        flag = 1;
    }

    printf("1\n");

    // Mapeamos el segmento de memoria compartida en el espacio de direcciones del proceso
    buffer = mmap(NULL, sizeof(Sistema), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (buffer == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    mem = buffer;


    alarma.sa_handler = handler_SIGINT;
    sigemptyset(&alarma.sa_mask);
    alarma.sa_flags = 0;

    if (sigaction(SIGALRM, &alarma, NULL) < 0)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    alarm(lag);

    // Si es el primer proceso minero
    if (flag == 0)
    {
        printf("2\n");
        // Fijamos el tamaño del segmento de memoria compartida
        if (ftruncate(fd_shm, sizeof(Sistema)) == -1)
        {
            perror("ftruncate");
            exit(EXIT_FAILURE);
        }

        // Inicializamos los semáforos
        if (sem_init(&buffer->sem_empty, 1, 0) == -1)
        {
            perror("sem_init");
            munmap(buffer, sizeof(Sistema));
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
        if (sem_init(&buffer->sem_fill, 1, 0) == -1)
        {
            perror("sem_init");
            munmap(buffer, sizeof(Sistema));
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
        if (sem_init(&buffer->sem_mutex, 1, 1) == -1)
        {
            perror("sem_init");
            munmap(buffer, sizeof(Sistema));
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }

        // Inicializamos el sistema
        buffer->cont = 0;
        buffer->bloque_act.contador = 0;
        buffer->num_minando = 0;
    }
    // Si es el segundo proceso minero
    else if (flag == 1)
    {
    }

    sem_wait(&buffer->sem_mutex);
    if (buffer->cont == MAX_MINEROS - 1)
    {
        printf("No se pueden aceptar más mineros\n");
        sem_post(&buffer->sem_mutex);
        exit(EXIT_FAILURE);
    }
    printf("3\n");
    buffer->datos_minero[buffer->cont].pid_minero = getpid();
    buffer->datos_minero[buffer->cont].monedas = 0;
    buffer->datos_minero[buffer->cont].votos = 0;
    buffer->cont++;
    sem_post(&buffer->sem_mutex);
    buffer->ult_Bloque_res.id = 0;

    error = pipe(pipe_minreg);
    if (error == -1)
    {
        perror("pipe\n");
        exit(EXIT_FAILURE);
    }

    error = pipe(pipe_regmin);
    if (error == -1)
    {
        perror("pipe\n");
        exit(EXIT_FAILURE);
    }

    printf("4\n");
    pid_secundario = fork();

    while (1)
    {

        // Proceso Minero
        if (pid_secundario > 0)
        {
            printf("5\n");
            act.sa_handler = handler_SIGINT;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;

            if (sigaction(SIGINT, &act, NULL) < 0)
            {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }

            // Cerramos el extremo de lectura del pipe
            close(pipe_minreg[0]);
            // ceramos el extremo de escritura del pipe
            close(pipe_regmin[1]);

            if (flag == 0)
            {
                // Inicializamos el bloque actual
                buffer->bloque_act.id = 0;
                buffer->bloque_act.contador = 0;
                buffer->bloque_act.num_votos_positivos = 0;
                buffer->bloque_act.num_votos_totales = 0;
                bandera_u2 = FALSE;
                senal_perdida1 = FALSE;
                senal_perdida2 = FALSE;

                printf("6\n");
                sem_wait(&buffer->sem_empty);
                sem_post(&buffer->sem_fill);
                enviar_senal_mineros(buffer);
            }
            else if (flag == 1)
            {

                // Configuración del manejador de señal para
                act_usr.sa_handler = handler_sigusr1;
                sigemptyset(&(act_usr.sa_mask));
                act_usr.sa_flags = 0;

                if (sigaction(SIGUSR1, &act_usr, NULL) < 0)
                {
                    perror("sigaction");
                    exit(EXIT_FAILURE);
                }

                sigfillset(&act_usr.sa_mask);         // Se vacía el conjunto de señales.
                sigdelset(&act_usr.sa_mask, SIGUSR1); // Se agrega SIGUSR1 del conjunto de señales.
                sigdelset(&act_usr.sa_mask, SIGINT);  // Se agrega SIGINT del conjunto de señales.
                sigdelset(&act_usr.sa_mask, SIGALRM); // Se agrega SIGALRM del conjunto de señales.

                // Bloqueamos todas las señales excepto SIGUSR1 y SIGINT
                if (sigprocmask(SIG_BLOCK, &act_usr.sa_mask, NULL) < 0)
                {
                    perror("sigprocmask");
                    exit(EXIT_FAILURE);
                }
                printf("7\n");
                sem_post(&buffer->sem_empty);
                if (senal_perdida1 == TRUE)
                {
                    senal_perdida1 = FALSE;
                }
                else
                {
                    sigsuspend(&act_usr.sa_mask);
                    senal_perdida1 = FALSE;
                }
            }
            sem_wait(&buffer->sem_mutex);
            buffer->bloque_act.cartera_min_act[buffer->bloque_act.contador].pid_minero = getpid();
            buffer->bloque_act.contador++;
            sem_post(&buffer->sem_mutex);
            printf("8\n");

            sem_wait(&buffer->sem_mutex);
            buffer->num_minando++;
            sem_post(&buffer->sem_mutex);
            proceso_de_minado(buffer, num_hilos, objetivo);
            sem_wait(&buffer->sem_mutex);
            buffer->num_minando--;
            sem_post(&buffer->sem_mutex);

            // Esperamos a que todos terminen la ronda
            while (buffer->num_minando != 0)
            {
                usleep(1000000);
            }

            // Envio de bloque resuelto al proceso Registrador
            write(pipe_minreg[1], &buffer->ult_Bloque_res, sizeof(Bloque));
            objetivo = buffer->ult_Bloque_res.solucion;
        }

        // Proceso Registrador
        else if (pid_secundario == 0)
        {
            printf("9\n");

            if (term == TRUE)
            {
                close(pipe_regmin[1]);
                close(pipe_minreg[0]);
                close(pipe_minreg[1]);
                close(pipe_regmin[0]);
                printf("\n\n\nLLEGO AQUI 1\n\n\n");
                exit(EXIT_SUCCESS);
            }

            error = registrador(pipe_minreg, pipe_regmin);
            if (error)
            {
                printf("\n\n\nLLEGO AQUI 3\n\n\n");
                exit(EXIT_FAILURE);
            }

            printf("term es : %d\n", term);

            if (term == TRUE)
            {
                close(pipe_regmin[1]);
                close(pipe_minreg[0]);
                close(pipe_minreg[1]);
                close(pipe_regmin[0]);
                printf("\n\n\nLLEGO AQUI 2\n\n\n");
                exit(EXIT_SUCCESS);
            }
        }

        //} // Fin del bucle infinito
        printf("10: %d\n", getpid());
    }
    // esto luego se quita

    return 0;
}

int proceso_de_minado(Sistema *buffer, int num_hilos, int objetivo)
{
    BOOL flag = FALSE;
    int resultado = 0;
    struct sigaction act_usr2;
    ThreadArgs *thread_Args;

    thread_Args = (ThreadArgs *)malloc(num_hilos * sizeof(ThreadArgs));
    if (!thread_Args)
    {
        fprintf(stderr, "Error reservando memoria para final_args\n");
        exit(EXIT_FAILURE);
    }

    printf(" mindado 1: %d\n", getpid());
    // Configuración del manejador de señal para SIGUSR2
    act_usr2.sa_handler = handler_sigusr2;
    sigemptyset(&(act_usr2.sa_mask));
    act_usr2.sa_flags = 0;

    if (sigaction(SIGUSR2, &act_usr2, NULL) < 0)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    printf(" mindado 2: %d\n", getpid());
    ronda(num_hilos, objetivo, &flag, &resultado, thread_Args);

    if (sem_trywait(&buffer->sem_fill) == 0) // Es el minero que ha encontrado la solucion (proceso ganador)
    {
        printf(" mindado 3: %d\n", getpid());
        for (int i = 0; i < buffer->bloque_act.contador; i++)
        {
            // Enviamos señal a todos los mineros para que terminen
            if (buffer->bloque_act.cartera_min_act[i].pid_minero != getpid())
            {
                kill(buffer->bloque_act.cartera_min_act[i].pid_minero, SIGUSR2);
            }
        }
        // Actualizamos el bloque ganador
        buffer->bloque_act.id = buffer->ult_Bloque_res.id + 1;
        buffer->bloque_act.objetivo = objetivo;
        buffer->bloque_act.solucion = resultado;
        buffer->bloque_act.pid_minero_ganador = getpid();
        buffer->bloque_act.num_votos_totales = 0;
        buffer->bloque_act.num_votos_positivos = 0;

        sem_wait(&buffer->sem_empty);
        for (int i = 0; i < buffer->bloque_act.contador; i++)
        {
            // Enviamos señal a todos los mineros para que terminen
            if (buffer->bloque_act.cartera_min_act[i].pid_minero != getpid())
            {

                kill(buffer->bloque_act.cartera_min_act[i].pid_minero, SIGUSR2);
                printf("enviando señal SIGUSR2 a : %d\n", buffer->bloque_act.cartera_min_act[i].pid_minero);
            }
        }
        printf(" mindado 5: %d\n", getpid());
        // Espera a que todos los procesos hagan su voto
        while (buffer->bloque_act.num_votos_totales != (buffer->bloque_act.contador - 1))
        {
            printf("Num contador: %d\n", buffer->bloque_act.contador);
            printf("Num votos totales: %d\n", buffer->bloque_act.num_votos_totales);
            usleep(1000000);
        }
        printf(" mindado 6: %d\n", getpid());
        // Ha sido aceptado por todos los mineros
        if (buffer->bloque_act.num_votos_positivos == buffer->bloque_act.num_votos_totales)
        {
            printf(" mindado 7: %d\n", getpid());
            for (int i = 0; i < buffer->cont - 1; i++)
            {
                if (buffer->datos_minero[i].pid_minero == getpid())
                {
                    buffer->datos_minero[i].monedas++;
                    buffer->datos_minero[i].votos += buffer->bloque_act.num_votos_positivos;
                }
            }

            for(int i = 0; i < buffer->bloque_act.contador; i++)
            {
                if(buffer->bloque_act.cartera_min_act[i].pid_minero == getpid())
                {
                    buffer->bloque_act.cartera_min_act[i].monedas++;
                }
            }
        }
        buffer->ult_Bloque_res = buffer->bloque_act;
    }

    // Es un proceso perdedor
    else
    {
        printf(" mindado 8: %d\n", getpid());

        struct sigaction act;
        sigset_t set;

        // Establecer el manejador de señales para SIGUSR2
        act.sa_handler = handler_sigPerdida;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        sigaction(SIGUSR2, &act, NULL);

        // Bloquear todas las señales excepto SIGUSR2
        sigfillset(&set);
        sigdelset(&set, SIGUSR2);
        sigdelset(&set, SIGINT);
        sigdelset(&set, SIGALRM);
        if (sigprocmask(SIG_BLOCK, &set, NULL) < 0)
        {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }

        sem_post(&buffer->sem_empty);
        // Esperar a SIGUSR2
        printf("Esperando señal SIGUSR2: %d\n", getpid());
        if (senal_perdida2 == TRUE)
        {
            senal_perdida2 = FALSE;
        }
        else
        {
            sigsuspend(&set);
            senal_perdida2 = FALSE;
        }
        printf(" mindado 9: %d\n", getpid());
        // Votamos
        if (pow_hash(buffer->bloque_act.solucion) == buffer->bloque_act.objetivo)
        {
            sem_wait(&buffer->sem_mutex);
            buffer->bloque_act.num_votos_positivos++;
            buffer->bloque_act.num_votos_totales++;
            printf("He votadoo, numvotostot: %d\n", buffer->bloque_act.num_votos_totales);
            sem_post(&buffer->sem_mutex);
        }
        else
        {
            sem_wait(&buffer->sem_mutex);
            buffer->bloque_act.num_votos_totales++;
            printf("He votadoo mal, numvotostot: %d\n", buffer->bloque_act.num_votos_totales);
            sem_post(&buffer->sem_mutex);
        }
    }

    free(thread_Args);
    return 0;
}

int ronda(int num_hilos, int objetivo, BOOL *flag, int *resultado, ThreadArgs *thread_Args)
{
    int j, range_acc = 0, range, error, z;
    pthread_t h1[num_hilos];

    range = POW_LIMIT / num_hilos;

    for (j = 0; j < num_hilos; j++)
    {
        thread_Args[j].inicio = range_acc;
        thread_Args[j].fin = range_acc + range;
        thread_Args[j].objetivo = objetivo;
        thread_Args[j].finalizado = flag;
        thread_Args[j].resultado = resultado;

        range_acc += range + 1;

        // Crear hilo con una copia diferente de la estructura FinalArgs
        error = pthread_create(&h1[j], NULL, minar, &thread_Args[j]);
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

void *minar(void *argumento)
{
    int inicio, fin, i;
    ThreadArgs *thread_Args = argumento;
    inicio = thread_Args->inicio;
    fin = thread_Args->fin;

    for (i = inicio; i < fin && *(thread_Args->finalizado) == FALSE && bandera_u2 == FALSE; i++)
    {
        if (pow_hash(i) == thread_Args->objetivo)
        {
            *(thread_Args->finalizado) = TRUE;
            *(thread_Args->resultado) = i;
        }
    }

    pthread_exit(NULL);
}

void enviar_senal_mineros(Sistema *buffer)
{
    printf("enviando señal\n");
    printf("contador de sistema %d\n", buffer->cont);
    printf("contador de bloque %d\n", buffer->bloque_act.contador);

    int i;
    for (i = 0; i < buffer->cont; i++)
    {
        if (buffer->datos_minero[i].pid_minero != getpid())
            kill(buffer->datos_minero[i].pid_minero, SIGUSR1);
        printf("señal enviada \n");
    }
}

void handler_sigusr1(int sig)
{
    senal_perdida1 = TRUE;
}

void handler_sigusr2(int sig)
{
    bandera_u2 = TRUE;
}

void handler_sigPerdida(int sig)
{
    senal_perdida2 = TRUE;
}

void handler_SIGINT(int sig)
{
    printf("ESTOY EN EN SIGINT\n");

    Sistema *del = (Sistema *)mem;

    term = TRUE;

    // espera la finalizacion de su proceso Registrador
    wait(NULL);
    printf("REGISTRADOR TERMINADO\n");

    // Eliminamos el minero del sistema
    sem_wait(&del->sem_mutex);
    for (int i = 0; i < del->cont; i++)
    {
        if (del->datos_minero[i].pid_minero == getpid())
        {
            // Si no es el último minero, lo sustituimos por el último, si lo es, simplemente lo eliminamos
            if (del->datos_minero[i].pid_minero != del->datos_minero[del->cont - 1].pid_minero)
            {
                del->datos_minero[i] = del->datos_minero[del->cont - 1];
            }
            del->cont--;
        }
    }

    printf("COnta va por: %d\n", del->cont);
    // Si no quedan mineros, cerramos el sistema
    if (del->cont == 0)
    {
        sem_post(&del->sem_mutex);
        printf("SOY EL ULTIMO MINERO\n");
        // Cerramos y eliminamos semáforos (solo lo hace el proceso monitor)
        if (sem_destroy(&del->sem_empty) == -1)
        {
            perror("sem_destroy");
            munmap(del, sizeof(Sistema));
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
        if (sem_destroy(&del->sem_fill) == -1)
        {
            perror("sem_destroy");
            munmap(del, sizeof(Sistema));
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
        if (sem_destroy(&del->sem_mutex) == -1)
        {
            perror("sem_destroy");
            munmap(del, sizeof(Sistema));
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }

        // Desmapeamos y cerramos el segmento de deloria compartida
        munmap(del, sizeof(Sistema));
        close(fd_shm);

        // Eliminamos la deloria compartida
        if (shm_unlink(SHM_NAME) == -1)
        {
            perror("shm_unlink");
            exit(EXIT_FAILURE);
        }
    }

    // Si no el ultimo minero, desmapeamos y cerramos el segmento de deloria compartida
    else
    {
        sem_post(&del->sem_mutex);
        munmap(del, sizeof(Sistema));
    }

    exit(EXIT_SUCCESS);
}