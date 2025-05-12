#include "func.h" // Incluye el archivo de cabecera "func.h" que contiene las declaraciones de funciones

int main(int argc, char *argv[])
{

    int shmid;              // Identificador de la memoria compartida
    key_t key = 1234;       // Clave única para la memoria compartida
    char *shm;              // Puntero al área de memoria compartida
    int *shared_variable;   // Variable compartida
    int num_proc, max_secs; // Declaración de variables para el número de procesos y el número máximo de segundos
    sem_t *sem_candi = NULL, *sem = NULL, *sem_u1 = NULL, *sem_rondas = NULL, *sem_senal = NULL;
    struct sigaction act_int;

    // Verifica si se proporciona el número correcto de argumentos en la línea de comandos
    if (argc != 3)
    {
        perror("Error en el numero de argumentos\n"); // Imprime un mensaje de error si no se proporcionan los argumentos adecuados
        return EXIT_FAILURE;                          // Termina el programa con un código de error
    }

    // Creamos la memoria compartida
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Adjuntamos el segmento de memoria compartida al espacio de direcciones del proceso
    if ((shm = shmat(shmid, NULL, 0)) == (char *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
        act_int.sa_handler = handler;
    sigemptyset(&(act_int.sa_mask));
    act_int.sa_flags = 0;

    // Configuración del manejador de señal
    if (sigaction(SIGINT, &act_int, NULL) < 0)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGALRM, &act_int, NULL) < 0)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Convertimos la dirección de la memoria compartida a un puntero entero
    shared_variable = (int *)shm;
    *shared_variable = 0;

    

    // Convierte los argumentos de la línea de comandos a enteros
    num_proc = atoi(argv[1]); // Convierte el primer argumento a un entero
    max_secs = atoi(argv[2]); // Convierte el segundo argumento a un entero

    alarm(max_secs);        // Establece una alarma que generará una señal SIGALRM después de un número máximo de segundos

    sem_unlink(SEM_CANDI);
    sem_unlink(SEM_NAME);
    sem_unlink(SEM_USR1);
    sem_unlink(SEM_RONDAS);
    sem_unlink(SEM_SENAL);

    if ((sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_u1 = sem_open(SEM_USR1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, num_proc)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_candi = sem_open(SEM_CANDI, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_rondas = sem_open(SEM_RONDAS, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, num_proc)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((sem_senal = sem_open(SEM_SENAL, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }


    // Crea los procesos votantes
    if (!crear_votantes(num_proc, sem, sem_candi, sem_u1, sem_rondas, sem_senal, shared_variable))
    {                        // Llama a la función crear_votantes para crear los procesos votantes
        return EXIT_FAILURE; // Termina el programa con un código de error
    }

    // Termina los procesos votantes
    terminar_votantes(num_proc);     // Llama a la función terminar_votantes para terminar los procesos votantes
    

    sem_close(sem_candi);
    sem_unlink(SEM_CANDI);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    sem_close(sem_u1);
    sem_unlink(SEM_USR1);
    sem_close(sem_rondas);
    sem_unlink(SEM_RONDAS);
    sem_close(sem_senal);
    sem_unlink(SEM_SENAL);

    // Desatachamos la memoria compartida del espacio de direcciones del proceso
    if (shmdt(shm) == -1)
    {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    // Eliminamos la memoria compartida
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS; // Termina el programa con éxito
}
