
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define MAX_CLIENTS 3

int clients[MAX_CLIENTS];
int num_clients = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast(int sender, char *message) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < num_clients; i++) {
        if (clients[i] != sender) {
            send(clients[i], message, strlen(message), 0);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *client_socket) {
    int socket_fd = *((int *)client_socket);
    char welcome_msg[100];

    snprintf(welcome_msg, sizeof(welcome_msg), "Welcome, User%d!\n", socket_fd);
    send(socket_fd, welcome_msg, strlen(welcome_msg), 0);

    char buffer[1024];
    int bytes_received;

    while ((bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_received] = '\0';

        // Broadcast the received message to all clients
        broadcast(socket_fd, buffer);
    }

    // Cli disconnected
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] == socket_fd) {
            printf("Client %d disconnected.\n", i + 1);
            for (int j = i; j < num_clients - 1; j++) {
                clients[j] = clients[j + 1];
            }
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(socket_fd);
    free(client_socket);

    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error in socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error in binding");
        exit(1);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Error in listening");
        exit(1);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket == -1) {
            perror("Error in accepting");
            continue;
        }

        if (num_clients < MAX_CLIENTS) {
            pthread_t thread;
            int *new_client = malloc(sizeof(int));
            *new_client = client_socket;

            pthread_create(&thread, NULL, handle_client, (void *)new_client);

            pthread_mutex_lock(&clients_mutex);
            clients[num_clients++] = client_socket;
            pthread_mutex_unlock(&clients_mutex);

            printf("New client connected. Total clients: %d\n", num_clients);
        } else {
            printf("Max clients reached. Connection rejected.\n");
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}
