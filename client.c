#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

float get_cpu_usage() {
    FILE *fp;
    char buffer[128];
    float cpu_usage;

    fp = popen("top -bn1 | grep 'Cpu(s)' | sed 's/.*, *\\([0-9.]*\\)%* id.*/\\1/' | awk '{print 100 - $1}'", "r");
    if (fp == NULL) {
        perror("Falha ao executar comando");
        return 0.0;
    }

    fgets(buffer, sizeof(buffer), fp);
    cpu_usage = atof(buffer);
    pclose(fp);

    return cpu_usage;
}

long get_memory_usage() {
    FILE *fp;
    char buffer[128];
    long memory_usage;

    fp = popen("free | grep Mem | awk '{print $3}'", "r");
    if (fp == NULL) {
        perror("Falha ao executar comando");
        return 0;
    }

    fgets(buffer, sizeof(buffer), fp);
    memory_usage = atol(buffer) / 1024; // Converte para MB
    pclose(fp);

    return memory_usage;
}

long get_network_latency() {
    struct timespec start, end;
    long latency;

    clock_gettime(CLOCK_MONOTONIC, &start);
    system("ping -c 1 " SERVER_IP " > /dev/null"); // Executa o ping
    clock_gettime(CLOCK_MONOTONIC, &end);

    latency = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000; // Converte para milissegundos
    return latency;
}

void collect_metrics(int client_id, int interval) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha na criação do socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    while (1) {
        float cpu_usage = get_cpu_usage();
        long memory_usage = get_memory_usage();
        long network_latency = get_network_latency();

        snprintf(buffer, BUFFER_SIZE, "client_id=%d, cpu_usage=%.2f, memory_usage=%ld, network_latency=%ld", 
                 client_id, cpu_usage, memory_usage, network_latency);

        sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
        printf("Cliente %d enviou: %s\n", client_id, buffer);
        sleep(interval);
    }

    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <client_id> <interval>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_id = atoi(argv[1]);
    int interval = atoi(argv[2]);
    if (interval <= 0) {
        fprintf(stderr, "Intervalo deve ser um número positivo.\n");
        exit(EXIT_FAILURE);
    }

    collect_metrics(client_id, interval);
    return 0;
}

