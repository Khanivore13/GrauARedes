#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int client_id;
    float cpu_usage;
    long memory_usage;
    long network_latency;
} ClientMetrics;

ClientMetrics metrics[MAX_CLIENTS];

volatile sig_atomic_t stop;

void handle_signal(int signum) {
    stop = 1;
}

void print_dashboard() {
    system("clear");
    printf("\n=== Dashboard de Métricas ===\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (metrics[i].client_id != 0) {
            printf("Cliente %d: CPU: %.2f%%, Memória: %ld MB, Latência: %ld ms\n",
                   metrics[i].client_id, metrics[i].cpu_usage, metrics[i].memory_usage, metrics[i].network_latency);
        }
    }
    printf("==============================\n");
}

void store_metrics(char *buffer) {
    int client_id;
    float cpu_usage;
    long memory_usage, network_latency;

    sscanf(buffer, "client_id=%d, cpu_usage=%f, memory_usage=%ld, network_latency=%ld",
           &client_id, &cpu_usage, &memory_usage, &network_latency);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (metrics[i].client_id == 0 || metrics[i].client_id == client_id) {
            metrics[i].client_id = client_id;
            metrics[i].cpu_usage = cpu_usage;
            metrics[i].memory_usage = memory_usage;
            metrics[i].network_latency = network_latency;
            break;
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha na criação do socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha ao fazer bind");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_signal);

    while (!stop) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        buffer[n] = '\0';
        store_metrics(buffer);
        print_dashboard();
    }

    close(sockfd);
    return 0;
}
