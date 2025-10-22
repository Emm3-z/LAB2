// ---------- CONSUMIDOR 1 (SUMA) ----------
    for (int i = 0; i < 5; i++) {
        int fd = open(FIFO_FILE, O_RDONLY);
        if (fd < 0) {
            perror("Error abriendo FIFO en consumidor 1");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        read(fd, buffer, sizeof(buffer));
        close(fd);

        int a, b;
        sscanf(buffer, "%d %d", &a, &b);
        int suma = a + b;

        printf("[Consumidor 1 - Suma] %d + %d = %d\n", a, b, suma);
        usleep(200000);
    }
    exit(0);
}
else if (pid_cons2 == 0) {
    // ---------- CONSUMIDOR 2 (MULTIPLICACIÓN) ----------
    for (int i = 0; i < 5; i++) {
        int fd = open(FIFO_FILE, O_RDONLY);
        if (fd < 0) {
            perror("Error abriendo FIFO en consumidor 2");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        read(fd, buffer, sizeof(buffer));
        close(fd);

        int a, b;
        sscanf(buffer, "%d %d", &a, &b);
        int mult = a * b;

        printf("[Consumidor 2 - Multiplicación] %d * %d = %d\n", a, b, mult);
        usleep(200000);
    }
    exit(0);
}
