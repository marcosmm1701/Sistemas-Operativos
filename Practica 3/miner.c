/**
 * @file miner.c
 * @author Ignacio Serena y Marcos Muñoz.
 * @brief Realiza el proceso minero.
 * @version 1.0
 * @date 2024-04-14
 *
 */

#include "miner.h"

int main(int argc, char *argv[])
{

    int objetivo, j, resultado = 0;

    // Verificar que se hayan pasado los argumentos correctos
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <ROUNDS> <LAG>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int rounds = atoi(argv[1]); // Número de rondas
    int lag = atoi(argv[2]);    // Retraso en milisegundos entre cada ronda

    if (rounds <= 0 || rounds > MAX_ROUNDS)
    {
        fprintf(stderr, "El número de rondas debe estar entre 1 y %d\n", MAX_ROUNDS);
        exit(EXIT_FAILURE);
    }

    struct message msg;
    struct mq_attr *attr = NULL;
    attr = (struct mq_attr *)malloc(sizeof(struct mq_attr));
    attr->mq_maxmsg = 7;
    attr->mq_msgsize = sizeof(msg);
    mqd_t queue;

    queue = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, attr);
    if (queue == (mqd_t)-1)
    {
        perror("Error al abrir la cola de mensajes");
        exit(EXIT_FAILURE);
    }

    // Establecimiento del objetivo inicial
    objetivo = 0;

    printf("[%d] Generating blocks...\n", getpid());

    // Bucle para realizar las rondas
    for (int i = 0; i < rounds; i++)
    {

        // Resolución de la prueba de trabajo
        for (j = 0; j < POW_LIMIT; j++)
        {
            if (pow_hash(j) == objetivo)
            {
                resultado = j;
                break;
            }
        }

        // Envío del mensaje con el objetivo y la solución
        msg.objetivo = objetivo;
        msg.resultado = resultado;
        if (mq_send(queue, (char *)&msg, sizeof(msg), 0) == -1)
        {
            fprintf(stderr, " Error sending message \n");
            return EXIT_FAILURE;
        }

        // Espera entre rondas
        usleep(lag * 1000);

        // Actualización del objetivo para la siguiente ronda
        objetivo = resultado;
    }

    // Envío del bloque especial al finalizar
    msg.objetivo = -1;
    msg.resultado = -1;
    if (mq_send(queue, (char *)&msg, sizeof(msg), 0) == -1)
    {
        fprintf(stderr, " Error sending message \n");
        return EXIT_FAILURE;
    }

    printf("[%d] Finishing\n", getpid());

    free(attr);

    return 0;
}
