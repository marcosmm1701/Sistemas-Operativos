/**
 * @file func.c
 * @author Ignacio Serena y Marcos Muñoz
 * @brief Encargado del la administracion de procesos
 * @version 1.0
 * @date 2024-03-18
 *
 *
 */

#include "func.h" // Incluye el archivo de cabecera func.h que contiene las declaraciones de las funciones y las constantes utilizadas en el programa.


/**
 * @brief Función para crear procesos votantes.
 *
 * @param num_proc Número de procesos votantes a crear.
 * @param sem Semáforo para la sincronización de procesos.
 * @param sem_candi Semáforo para controlar la selección del candidato.
 * @param sem_usr Semáforo para la comunicación entre procesos votantes.
 * @param sem_rondas Semáforo para controlar el inicio de las rondas.
 * @param sem_senal Semáforo para la recepción de señales entre procesos.
 * @param v_global Variable global compartida para conteo de votos.
 * @return BOOL Verdadero si la función se ejecutó correctamente, falso en caso contrario.
 */BOOL crear_votantes(int num_proc, sem_t *sem, sem_t *sem_candi, sem_t *sem_usr, sem_t *sem_rondas, sem_t *sem_senal, int *v_global)
{
    pid_t pid;           // Variable para almacenar el PID de un proceso.
    int i, valor = 1, z; // Puntero a la variable compartida
    int shmid;
    key_t key = 999; // Clave para identificar la memoria compartida
    int *estatica;

    fclose(fopen(FICHERO, "w"));
    fclose(fopen(FPRUEBA, "w"));

    // Creamos la memoria compartida
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Adjuntamos la variable compartida a nuestro espacio de memoria
    if ((estatica = shmat(shmid, NULL, 0)) == (int *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }


    // Ciclo para crear procesos votantes.
    for (i = 0; i < num_proc; i++)
    {
        pid = fork(); // Se crea un nuevo proceso.

        if (pid == -1) // Error al crear el proceso hijo.
        {
            perror("Error al crear el proceso votante");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) // Proceso hijo (votante)
        {
            while (1)
            {

                sem_post(sem_senal);

                *v_global = 0;

                hijos(num_proc, sem, sem_candi, sem_usr, sem_rondas, v_global, estatica);
                fclose(fopen(FICHERO, "w"));
                fclose(fopen(FPRUEBA, "w"));
                do
                {
                    usleep(1000); // Espera a que todos los hijos hayan terminado para empezar la siguiente ronda
                    sem_getvalue(sem_rondas, &valor);
                } while (valor != 0);

                *estatica = *estatica + 1;
                while (*estatica < num_proc) // evita que las rondas se solapen. Que se haga sempost(rondas), sin haber salido todos del do while
                    usleep(1000);
                sem_post(sem_rondas); // Ponen de nuevo a sem_rondas = al numero de procesos. Para la siguiente ronda.

                usleep(250000); // Espera 250000 microsegundos, equivalente a 250 milisegundos
            }
        }
    }

    while (1)
    {

        for (z = 0; z < num_proc; z++)
            sem_wait(sem_senal);

        sem_post(sem_candi); // Para que solo 1 proceso sea el candidato

        // Envía una señal a los procesos votantes
        if (!enviar_senal_votantes(sem_usr, sem_rondas))
        {                        // Llama a la función enviar_senal_votantes para enviar una señal a los procesos votantes
            return EXIT_FAILURE; // Termina el programa con un código de error
        }
    }

    return TRUE; // Retorna verdadero indicando que la función se ejecutó correctamente
}

/**
 * @brief Función para ejecutar las acciones de los procesos hijos.
 *
 * @param num_proc Número total de procesos.
 * @param sem Semáforo para la sincronización de procesos.
 * @param sem_candi Semáforo para controlar la selección del candidato.
 * @param sem_usr Semáforo para la comunicación entre procesos votantes.
 * @param sem_rondas Semáforo para controlar el inicio de las rondas.
 * @param v_global Variable global compartida para conteo de votos.
 * @param estatica Variable compartida para evitar solapamiento de rondas.
 * @return int Código de error, 0 si éxito, otros valores si hay errores.
 */
int hijos(int num_proc, sem_t *sem, sem_t *sem_candi, sem_t *sem_usr, sem_t *sem_rondas, int *v_global, int *estatica)
{

    FILE *votos, *f;
    char *line; // Variable para almacenar una línea de votos.
    line = (char *)calloc(((num_proc * 2) + 1), sizeof(char));
    struct sigaction act_usr; // Estructuras para configurar los manejadores de señales.
    int shmid;
    key_t key = 666; // Clave para identificar la memoria compartida
    int *contador;

    // Creamos la memoria compartida
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Adjuntamos la variable compartida a nuestro espacio de memoria
    if ((contador = shmat(shmid, NULL, 0)) == (int *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Configuración del manejador de señal para
    act_usr.sa_handler = handler_usr;
    sigemptyset(&(act_usr.sa_mask));
    act_usr.sa_flags = 0;

    *contador = 0;

    f = fopen(FICHERO, "a");
    if (!f)
    {
        printf("Error abriendo el archivo\n");
        exit(EXIT_FAILURE);
    }
    fprintf(f, "%d\n", getpid()); // Se escribe el PID del proceso votante en el archivo.
    fclose(f);                    // Se cierra el archivo.


    if (sigaction(SIGUSR1, &act_usr, NULL) < 0)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sigfillset(&act_usr.sa_mask);         // Se llena el conjunto de señales.
    sigdelset(&act_usr.sa_mask, SIGUSR1); // Se elimina SIGUSR1 del conjunto de señales.
    sigdelset(&act_usr.sa_mask, SIGTERM);



    sem_trywait(sem_usr);         // Decrementa el valor del semaforo y avanza a la suspension inmediatamente. Esta implementación es indispensable para evitar errores de concurrencia muy frecuentes jajaja
    sigsuspend(&act_usr.sa_mask); // El proceso espera a recibir la señal SIGUSR1.


    /*Poceso candidato*/
    int v;
    sem_getvalue(sem_candi, &v);

    if (v == 2)
        sem_wait(sem_candi);

    if (sem_trywait(sem_candi) == 0)
    {

        for (int j = 0; j < num_proc - 1; j++)
        {
            sem_post(sem_usr);
        }

        if (!inicio_votacion(sem_usr, sem)) // envia SIGUSR2
            return FALSE;

        while (*contador < num_proc - 1) // Comprueba si se han hecho todos los votos.
            usleep(1000);                // Se suspende la ejecución del proceso durante 1000 microsegundos.

        votar(v_global); // voto del candidato

        printf("Candidate %d => [ ", getpid()); // Imprime el mensaje del candidato con su identificador
        votos = fopen(FVOTOS, "r");             // Abre el archivo de votos en modo de lectura

        if (fread(line, sizeof(char), (num_proc * 2), votos) == -1)
        {
            perror("Erroe en el fread");
            exit(EXIT_FAILURE);
        }
        line[strlen(line)] = '\0';
        printf("%s", line); // Imprime cada línea del archivo de votos
        fclose(votos);

        if (*v_global <= 0)            // Si hay más votos negativos que positivos
            printf("] => Rejected\n"); // Imprime el mensaje indicando que el candidato ha sido rechazado
        else                           // Si hay más votos positivos que negativos o la misma cantidad
            printf("] => Accepted\n");
    }

    /*Procesos votantes*/
    else
    {

        fclose(fopen(FVOTOS, "w")); // Vacia el fichero
        if (sigaction(SIGUSR2, &act_usr, NULL) < 0)
        {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        sigfillset(&act_usr.sa_mask);         // Se llena el conjunto de señales.
        sigdelset(&act_usr.sa_mask, SIGUSR2); // Se elimina SIGUSR2 del conjunto de señales.
        sigdelset(&act_usr.sa_mask, SIGTERM);



        sem_trywait(sem_usr);         // Decrementa el valor del semaforo en 1 y se suspende
        sigsuspend(&act_usr.sa_mask); // El proceso espera a recibir la señal SIGUSR2.


        if (!sem_wait(sem)) // Entra el primero y los demás se quedan bloqueados hasta que haya terminado de escribir
        {

            if (votar(v_global) == -1)
            {
                sem_close(sem_candi);
                sem_unlink(SEM_CANDI);
                sem_close(sem);
                sem_unlink(SEM_NAME);
                sem_close(sem_usr);
                sem_unlink(SEM_USR1);
                sem_close(sem_rondas);
                sem_unlink(SEM_RONDAS);
                free(line);
                exit(EXIT_SUCCESS);
            }
            *contador = *contador + 1;
            sem_post(sem); // Pone sem a 1 para que vote el siguiente proceso votante.
        }
    }

    sem_wait(sem_rondas); // Decrementa el semaforo cada vez que termina un hijo.
    sem_post(sem_usr);
    free(line);
    *estatica = 0; // evita que las rondas se solapen
    return 0;
}

/**
 * @brief Función para enviar la señal SIGUSR1 a los procesos votantes.
 *
 * @param sem_usr Semáforo para la comunicación entre procesos votantes.
 * @param sem_candi Semáforo para controlar la selección del candidato.
 * @return BOOL Verdadero si la función se ejecutó correctamente, falso en caso contrario.
 */
BOOL enviar_senal_votantes(sem_t *sem_usr, sem_t *sem_candi)
{
    pid_t pid;
    char line[20];
    FILE *f;
    int valor;

    // Verifica si el puntero al archivo es nulo
    f = fopen(FICHERO, "r");
    if (!f)
    {
        printf("Error abriendo el archivo\n"); // Imprime un mensaje de error
        return FALSE;                          // Retorna falso indicando un error
    }

    do
    {
        usleep(10000); // Espera a que todos los pids esten escritos en el fichero y esperando SIGUSR1
        sem_getvalue(sem_usr, &valor);
    } while (valor != 0);

    while (fgets(line, sizeof(line), f)) // Mientras se pueda leer un PID del archivo
    {
        line[strlen(line) - 1] = '\0';
        pid = atoi(line);

        kill(pid, SIGUSR1); // Envía la señal SIGUSR1 al proceso con el PID especificado
    }
    fclose(f);

    return TRUE; // Retorna verdadero indicando que la función se ejecutó correctamente
}

/**
 * @brief Función para esperar a que terminen los procesos hijos.
 *
 * @param num_proc Número total de procesos.
 */
void terminar_votantes(int num_proc)
{
    for (int i = 0; i < num_proc; i++)
        wait(NULL); // No hace nada, solo espera a que terminen todos los procesos hijos
}

/**
 * @brief Manejador de señales para terminar los procesos hijos.
 *
 * @param sig Señal recibida.
 */
void handler(int sig)
{
    FILE *f = fopen(FICHERO, "r"); // Abre el archivo en modo de lectura
    pid_t pid;                     // Variable para almacenar el PID de un proceso
    sem_unlink(SEM_CANDI);

    sem_unlink(SEM_NAME);

    sem_unlink(SEM_USR1);

    sem_unlink(SEM_RONDAS);

    sem_unlink(SEM_SENAL);
    while (fread(&pid, sizeof(pid_t), 1, f) == 1) // Mientras se pueda leer un PID del archivo
    {
        kill(pid, SIGTERM); // Envía la señal SIGTERM al proceso con el PID especificado
    }
    fclose(f); // Cierra el archivo
    printf("Finishing by signal\n"); // Imprime un mensaje indicando que el programa ha terminado debido a una señal
    exit(EXIT_SUCCESS);
}

/**
 * @brief Función para iniciar la votación enviando la señal SIGUSR2 a los procesos votantes.
 *
 * @param sem_usr Semáforo para la comunicación entre procesos votantes.
 * @param sem Semáforo para la sincronización de procesos.
 * @return BOOL Verdadero si la función se ejecutó correctamente, falso en caso contrario.
 */
BOOL inicio_votacion(sem_t *sem_usr, sem_t *sem)
{
    pid_t pid;
    char line[20];
    FILE *f;
    int valor;

    // Verifica si el puntero al archivo es nulo
    f = fopen(FICHERO, "r");
    if (!f)
    {
        printf("Error abriendo el archivo\n"); // Imprime un mensaje de error
        return FALSE;                          // Retorna falso indicando un error
    }

    do
    {
        usleep(10000); // Espera a que todos los procesos, estén esperando a SIGUSR2
        sem_getvalue(sem_usr, &valor);
    } while (valor != 0);

    while (fgets(line, sizeof(line), f) != NULL) // Mientras se pueda leer un PID del archivo
    {
        line[strlen(line) - 1] = '\0';
        pid = atoi(line);

        if (pid != getpid())
        {
            if (kill(pid, SIGUSR2) == -1) // Envía la señal SIGUSR2 al proceso con el PID especificado
            {
                printf("Error en el kill\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    sem_post(sem); // Pone el semaforo a 1 para que el primer votante haga su voto.
    fclose(f);
    return TRUE; // Retorna verdadero indicando que la función se ejecutó correctamente
}

/**
 * @brief Función para que un proceso votante emita su voto.
 *
 * @param v_global Variable global compartida para conteo de votos.
 * @return int Código de error, 0 si éxito, otros valores si hay errores.
 */
int votar(int *v_global)
{
    FILE *votos;
    int random;
    char caracter;

    votos = fopen(FVOTOS, "a");
    if (votos == NULL)
    {
        perror("Error al abrir el archivo de votos");
        return -1;
    }

    srand(time(NULL));
    random = (int)((int)floor(getpid() * rand() * sqrt(2)) % 2);
    if (!random) // Si el número aleatorio es 0.
    {

        *v_global = *v_global + 1; // Se incrementa el contador de votos afirmativos.
        caracter = 'Y';
    }
    else // Si el número aleatorio es 1.
    {

        *v_global = *v_global - 1; // Se incrementa el contador de votos negativos.
        caracter = 'N';
    }

    if (fwrite(&caracter, sizeof(caracter), 1, votos) != 1)
    {
        perror("Error al escribir en el archivo de votos");
        fclose(votos);
        return -1;
    }
    caracter = ' ';
    if (fwrite(&caracter, sizeof(caracter), 1, votos) != 1)
    {
        perror("Error al escribir en el archivo de votos");
        fclose(votos);
        return -1;
    }
    fclose(votos);
    return 0;
}

/**
 * @brief Manejador de señales para los procesos hijos.
 *
 * @param sig Señal recibida.
 */
void handler_usr(int sig)
{
}
