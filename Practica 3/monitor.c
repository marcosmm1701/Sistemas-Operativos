/**
 * @file monitor.c
 * @author Ignacio Serena y Marcos Muñoz.
 * @brief Realiza el proceso de monitorización.
 * @version 1.0
 * @date 2024-04-14
 *
 */

#include "monitor.h"

/**
 * @brief  Esta función se encarga de comprobar los bloques recibidos a través de una cola de mensajes.
 *
 * @param lag Retraso en milisegundos entre cada comprobación.
 * @param buffer Puntero a la estructura de datos compartida.
*/
void comprobador(int lag, DatosBloques *buffer)
{
  mqd_t queue;
  int contador = 0; // indice de escritura en el buffer

  struct message *mensaje = malloc(sizeof(struct message)); // Puntero a la estructura message
  if (mensaje == NULL)
  {
    munmap(buffer, sizeof(DatosBloques));
    shm_unlink(SHM_NAME);
    perror("Error al asignar memoria para el mensaje");
    exit(EXIT_FAILURE);
  }

  queue = mq_open(MQ_NAME, O_RDONLY);
  if (queue == (mqd_t)-1)
  {
    munmap(buffer, sizeof(DatosBloques));
    shm_unlink(SHM_NAME);
    perror("Error al abrir la cola de mensajes en el comprobador");
    exit(EXIT_FAILURE);
  }

  Bloque *bloque = malloc(sizeof(Bloque));
  if (bloque == NULL)
  {
    mq_close(queue);
    mq_unlink(MQ_NAME);
    munmap(buffer, sizeof(DatosBloques));
    shm_unlink(SHM_NAME);
    exit(EXIT_FAILURE);
  }

  printf("[%d] Checking blocks...\n", getpid());

  while (1)
  {
    // Esperar a que haya bloques en la memoria compartida
    sem_wait(&buffer->sem_empty);
    sem_wait(&buffer->sem_mutex);

    ssize_t bytes_received = mq_receive(queue, (void *)mensaje, sizeof(struct message), NULL);

    bloque->objetivo = mensaje->objetivo;
    bloque->solucion = mensaje->resultado;

    if (bytes_received == -1)
    {
      perror("Error al recibir el mensaje de la cola de mensajes");
      // Cerrar la cola de mensajes y la memoria compartida
      mq_close(queue);
      mq_unlink(MQ_NAME);
      munmap(buffer, sizeof(DatosBloques));
      shm_unlink(SHM_NAME);
      free(mensaje);
      exit(EXIT_FAILURE);
    }
    else if (bytes_received != sizeof(struct message))
    {
      fprintf(stderr, "Tamaño de mensaje incorrecto recibido de la cola de mensajes\n");
      // Cerrar la cola de mensajes y la memoria compartida
      mq_close(queue);
      mq_unlink(MQ_NAME);
      munmap(buffer, sizeof(DatosBloques));
      shm_unlink(SHM_NAME);
      free(mensaje);
      exit(EXIT_FAILURE);
    }

    if (bloque->objetivo == -1)
    {

      buffer->bloques[contador] = *bloque;
      sem_post(&buffer->sem_mutex);
      sem_post(&buffer->sem_fill);
      break;
    }
    else
    {
      if (pow_hash(bloque->solucion) == bloque->objetivo)
      {
        bloque->es_correcto = 1;
      }
      else
      {
        bloque->es_correcto = 0;
      }
      buffer->bloques[contador] = *bloque;
    }

    contador++;
    if (contador == BUFFER_SIZE)
    {
      contador = 0;
    }

    // Realizar tareas de comprobación aquí
    sem_post(&buffer->sem_mutex);
    sem_post(&buffer->sem_fill);
    // Espera activa de lag milisegundos
    usleep(lag * 1000);
  };

  // Cerramos la cola de mensajes
  if (mq_close(queue) == -1)
  {
    perror("Error al cerrar la cola de mensajes");
    mq_unlink(MQ_NAME);
    munmap(buffer, sizeof(DatosBloques));
    shm_unlink(SHM_NAME);
    free(bloque);
    free(mensaje);
    exit(EXIT_FAILURE);
  }

  // Eliminamos la cola de mensajes
  if (mq_unlink(MQ_NAME) == -1)
  {
    perror("Error al eliminar la cola de mensajes");
    munmap(buffer, sizeof(DatosBloques));
    shm_unlink(SHM_NAME);
    free(bloque);
    free(mensaje);
    exit(EXIT_FAILURE);
  }

  free(bloque);
  free(mensaje);
  munmap(buffer, sizeof(DatosBloques));
  printf("[%d] Finishing\n", getpid());
  exit(EXIT_SUCCESS);
}

/**
 * @brief  Esta función se encarga de monitorizar los bloques recibidos a través de una memoria compartida.
 *
 * @param lag Retraso en milisegundos entre cada comprobación.
 * @param buffer Puntero a la estructura de datos compartida.
*/
void monitor(int lag, DatosBloques *buffer)
{
  int contador = 0; // indice de escritura en el buffer

  printf("[%d] Printing blocks...\n", getpid());

  // Monitorización y visualización de resultados
  while (1)
  {
    sem_wait(&buffer->sem_fill);
    sem_wait(&buffer->sem_mutex);
    // Simulación de visualización de resultados
    if (buffer->bloques[contador].objetivo == -1)
    {
      break;
    }

    else
    {

      if (buffer->bloques[contador].es_correcto == 1)
      {
        printf("Solution accepted: %08ld --> %08ld\n", buffer->bloques[contador].objetivo, buffer->bloques[contador].solucion);
      }
      else
      {
        printf("Solution rejected: %08ld !-> %08ld\n", buffer->bloques[contador].objetivo, buffer->bloques[contador].solucion);
      }

      contador++;
      if (contador == BUFFER_SIZE)
      {
        contador = 0;
      }
    }

    sem_post(&buffer->sem_mutex);
    sem_post(&buffer->sem_empty);

    // Espera activa de lag milisegundos
    usleep(lag * 1000);
  }
  printf("[%d] Finishing\n", getpid());
  return;
}

int main(int argc, char *argv[])
{
  int fd_shm;
  int flag = 0;
  DatosBloques *buffer;
  int lag = atoi(argv[1]); // Retraso

  // Si es el primero (Comprobador), creamos el segmento de memoria compartida
  fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd_shm == -1)
  {
    // Si la memoria compartida ya está creada (Monitor), la abrimos
    fd_shm = shm_open(SHM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_shm == -1)
    {
      perror("shm_open");
      exit(EXIT_FAILURE);
    }
    flag = 1;
  }

  else
  {
    // Fijamos el tamaño del segmento de memoria compartida
    if (ftruncate(fd_shm, sizeof(DatosBloques)) == -1)
    {
      perror("ftruncate");
      exit(EXIT_FAILURE);
    }
  }

  // Mapeamos el segmento de memoria compartida en el espacio de direcciones del proceso
  buffer = mmap(NULL, sizeof(DatosBloques), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
  if (buffer == MAP_FAILED)
  {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  // Si es el proceso comprobador
  if (flag == 0)
  {
    // Inicializamos los semáforos
    if (sem_init(&buffer->sem_empty, 1, BUFFER_SIZE) == -1)
    {
      perror("sem_init");
      munmap(buffer, sizeof(DatosBloques));
      close(fd_shm);
      shm_unlink(SHM_NAME);
      exit(EXIT_FAILURE);
    }
    if (sem_init(&buffer->sem_fill, 1, 0) == -1)
    {
      perror("sem_init");
      munmap(buffer, sizeof(DatosBloques));
      close(fd_shm);
      shm_unlink(SHM_NAME);
      exit(EXIT_FAILURE);
    }
    if (sem_init(&buffer->sem_mutex, 1, 1) == -1)
    {
      perror("sem_init");
      munmap(buffer, sizeof(DatosBloques));
      close(fd_shm);
      shm_unlink(SHM_NAME);
      exit(EXIT_FAILURE);
    }

    comprobador(lag, buffer);
  }
  // Si es el proceso monitor
  else
  {
    monitor(lag, buffer);
  }

  // Cerramos y eliminamos semáforos (solo lo hace el proceso monitor)
  if (sem_destroy(&buffer->sem_empty) == -1)
  {
    perror("sem_destroy");
    munmap(buffer, sizeof(DatosBloques));
    close(fd_shm);
    shm_unlink(SHM_NAME);
    exit(EXIT_FAILURE);
  }
  if (sem_destroy(&buffer->sem_fill) == -1)
  {
    perror("sem_destroy");
    munmap(buffer, sizeof(DatosBloques));
    close(fd_shm);
    shm_unlink(SHM_NAME);
    exit(EXIT_FAILURE);
  }
  if (sem_destroy(&buffer->sem_mutex) == -1)
  {
    perror("sem_destroy");
    munmap(buffer, sizeof(DatosBloques));
    close(fd_shm);
    shm_unlink(SHM_NAME);
    exit(EXIT_FAILURE);
  }

  // Desmapeamos y cerramos el segmento de memoria compartida (solo lo hace el proceso monitor)
  munmap(buffer, sizeof(DatosBloques));
  close(fd_shm);

  // Eliminamos la memoria compartida (solo lo hace el proceso monitor)
  if (shm_unlink(SHM_NAME) == -1)
  {
    perror("shm_unlink");
    exit(EXIT_FAILURE);
  }

  return 0;
}
