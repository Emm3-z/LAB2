#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define SHM_KEY 1234
#define FIFO_FILE "canal_fifo"
#define BUFFER_SIZE 128

typedef struct {
    int a;
    int b;
    int resultado_mul;
    int resultado_sum;
    int listo;          // 1 = productor puso nuevos datos
    int procesados;     // cuántos consumidores han terminado
} DatosCompartidos;

int main() {
    int shm_id;
    DatosCompartidos *mem;
    pid_t pid_prod, pid_cons1, pid_cons2;

    // Crear memoria compartida
    shm_id = shmget(SHM_KEY, sizeof(DatosCompartidos), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("Error creando memoria compartida");
        exit(1);
    }

    mem = (DatosCompartidos *) shmat(shm_id, NULL, 0);
    if (mem == (DatosCompartidos *) -1) {
        perror("Error asociando memoria compartida");
        exit(1);
    }

    // Inicializar variables
    mem->listo = 0;
    mem->procesados = 0;

    // Crear FIFO si no existe
    mkfifo(FIFO_FILE, 0666);

    pid_prod = fork();

    if (pid_prod == 0) {
        // ---------- PRODUCTOR ----------
        int fd = open(FIFO_FILE, O_WRONLY);
        if (fd < 0) {
            perror("Error abriendo FIFO para escritura");
            exit(1);
        }

        for (int i = 1; i <= 5; i++) {
            int a = i;
            int b = i + 2;

            char buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "%d %d", a, b);
            write(fd, buffer, strlen(buffer) + 1);
            printf("\n[Productor] Enviado por FIFO: %d %d\n", a, b);

            sleep(1);
        }

        close(fd);
        exit(0);
    } 
    else {
        pid_cons1 = fork();
        if (pid_cons1 == 0) {
            // ---------- CONSUMIDOR 1 (LECTOR Y SUMA) ----------
            int fd = open(FIFO_FILE, O_RDONLY);
            if (fd < 0) {
                perror("Error abriendo FIFO para lectura (consumidor 1)");
                exit(1);
            }

            char buffer[BUFFER_SIZE];
            for (int i = 0; i < 5; i++) {
                read(fd, buffer, sizeof(buffer));

                int a, b;
                sscanf(buffer, "%d %d", &a, &b);

                // Escribir los valores en la memoria compartida
                mem->a = a;
                mem->b = b;
                mem->procesados = 0;
                mem->listo = 1;

                // Calcular suma
                int suma = a + b;
                mem->resultado_sum = suma;
                mem->procesados += 1;

                // Esperar a que el otro consumidor haga la multiplicación
                while (mem->procesados < 2)
                    usleep(100000);

                printf("[Consumidor 1 - Suma]: %d + %d = %d | [Consumidor 2 - Multiplicación]: %d * %d = %d\n",
                       mem->a, mem->b, mem->resultado_sum,
                       mem->a, mem->b, mem->resultado_mul);

                mem->listo = 0;
            }

            close(fd);
            exit(0);
        } 
        else {
            pid_cons2 = fork();
            if (pid_cons2 == 0) {
                // ---------- CONSUMIDOR 2 (MULTIPLICACIÓN) ----------
                for (int i = 0; i < 5; i++) {
                    // Esperar que el consumidor 1 haya puesto nuevos datos
                    while (mem->listo == 0)
                        usleep(200000);

                    int a = mem->a;
                    int b = mem->b;
                    int mult = a * b;
                    mem->resultado_mul = mult;

                    mem->procesados += 1;

                    // Esperar que el consumidor 1 libere el flag
                    while (mem->listo == 1)
                        usleep(100000);
                }
                exit(0);
            }
        }
    }

    // ---------- PADRE ----------
    wait(NULL);
    wait(NULL);
    wait(NULL);

    shmdt(mem);
    shmctl(shm_id, IPC_RMID, NULL);
    unlink(FIFO_FILE);

    printf("\n[Padre] Todos los procesos finalizaron correctamente.\n");
    return 0;
}
