#include "registrador.h"

int registrador(int pipe_minreg[2], int pipe_regmin[2])
{
    Bloque bloque;
    FILE *fichero;
    char nom_fichero[MAX_STR];
    char *sol;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(pipe_minreg[0], &readfds);
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;

    close(pipe_regmin[0]);
    close(pipe_minreg[1]);

    sprintf(nom_fichero, "blockchain_%d.txt", getppid());
    fichero = fopen(nom_fichero, "a");
    int fichero_des = fileno(fichero);

    if (getppid() == 1)
        return EXIT_SUCCESS;

    // Esperamos hasta que haya datos disponibles en la tubería o hasta que expire el tiempo de espera
    int ret = select(pipe_minreg[0] + 1, &readfds, NULL, NULL, &timeout);
    if (ret == -1)
    {
        perror("select");
        fclose(fichero);
        close(fichero_des);
        exit(EXIT_FAILURE);
    }
    else if (ret == 0)
    {
        // Se ha alcanzado el tiempo límite de espera
        printf("Tiempo de espera excedido. No hay datos disponibles en la tubería.\n");
        fclose(fichero);
        close(fichero_des);
        exit(EXIT_FAILURE);
    }
    else
    {
        // Hay datos disponibles en la tubería para leer
        ssize_t bytesRead = read(pipe_minreg[0], &bloque, sizeof(Bloque));
        if (bytesRead == -1)
        {
            perror("read");
            fclose(fichero);
            close(fichero_des);
            return EXIT_FAILURE;
        }
        else if (bytesRead == 0)
        {
            printf("No hay datos disponibles en la tubería.\n");
            fclose(fichero);
            close(fichero_des);
            return EXIT_FAILURE;
        }
        else
        {
            // Se han leído datos correctamente
            if (bloque.num_votos_positivos == bloque.num_votos_totales)
            {
                sol = "(validated)";
            }
            else
            {
                sol = "(rejected)";
            }

            dprintf(fichero_des, "Id: %d\n", bloque.id);
            dprintf(fichero_des, "Winner: %d\n", bloque.pid_minero_ganador);
            dprintf(fichero_des, "Target: %d\n", bloque.objetivo);
            dprintf(fichero_des, "Solution: %d %s\n", bloque.solucion, sol);
            dprintf(fichero_des, "Votes: %d/%d\n", bloque.num_votos_positivos, bloque.num_votos_totales);
            dprintf(fichero_des, "Wallets: ");
            for (int i = 0; i < bloque.contador; i++)
            {
                dprintf(fichero_des, "%d:%d ", bloque.cartera_min_act[i].pid_minero, bloque.cartera_min_act[i].monedas);
            }
            dprintf(fichero_des, "\n\n");
        }
    }
    fclose(fichero);
    close(fichero_des);

    return EXIT_SUCCESS;
}