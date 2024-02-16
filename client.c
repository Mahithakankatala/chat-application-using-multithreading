#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define MAX_MSG_SIZE 1024

int client_socket;
char username[20];

void *receive_messages(void *arg) {
    char buffer[MAX_MSG_SIZE];
    int bytes_received;

    while (1) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("%s", buffer);
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error in socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error in connect");
        exit(1);
    }

    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strlen(username) - 1] = '\0'; // Remove the newline character

    printf("Connected to the server. Type 'exit' to quit.\n");

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, NULL);

    char message[MAX_MSG_SIZE];
    while (1) {
        printf("[%s] Enter message: ", username);
        fflush(stdout);  // Flush the standard output

        if (fgets(message, sizeof(message), stdin) == NULL) {
            break;  // Exit if there's an error reading input
        }

        if (strcmp(message, "exit\n") == 0) {
            send(client_socket, message, strlen(message), 0);
            break;
        }

        // Add the username to the message
        char formatted_message[MAX_MSG_SIZE];
        snprintf(formatted_message, sizeof(formatted_message), "[%s] %s", username, message);
        send(client_socket, formatted_message, strlen(formatted_message), 0);
    }

    close(client_socket);
    return 0;
}
