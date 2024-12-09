#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>

#define UDP_PORT 5555
#define TCP_PORT 6666
#define MAX_CLIENTS 32
#define BUFFER_SIZE 1024

typedef struct {
    char filename[BUFFER_SIZE];
    int client_port;
    struct sockaddr_in client_addr;
} client_data_t;

void *handle_client(void *arg) {
    client_data_t *client_data = (client_data_t *)arg;
    int data_socket, file_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // TCP data connection
    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket < 0) {
        perror("Socket creation failed");
        free(client_data);
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(client_data->client_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(data_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(data_socket);
        free(client_data);
        pthread_exit(NULL);
    }

    listen(data_socket, 1);
    printf("Waiting for client connection on port %d...\n", client_data->client_port);

    int client_sock = accept(data_socket, NULL, NULL);
    if (client_sock < 0) {
        perror("Accept failed");
        close(data_socket);
        free(client_data);
        pthread_exit(NULL);
    }

    file_fd = open(client_data->filename, O_RDONLY);
    if (file_fd < 0) {
        perror("File open failed");
        write(client_sock, "ERROR: File not found\n", 23);
    } else {
        while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
            send(client_sock, buffer, bytes_read, 0);
        }
        close(file_fd);
    }
    close(client_sock);
    close(data_socket);

    // TCP response connection
    int response_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (response_sock < 0) {
        perror("Response socket creation failed");
        free(client_data);
        pthread_exit(NULL);
    }

    struct sockaddr_in client_addr = client_data->client_addr;
    client_addr.sin_port = htons(TCP_PORT);

    if (connect(response_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Connect to client failed");
        close(response_sock);
        free(client_data);
        pthread_exit(NULL);
    }

    write(response_sock, "DONE", 4);
    close(response_sock);

    free(client_data);
    pthread_exit(NULL);
}

int main() {
    int udp_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP bind failed");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on UDP port %d...\n", UDP_PORT);

    while (1) {
        int n = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            perror("Failed to receive UDP packet");
            continue;
        }

        buffer[n] = '\0';
        printf("Received: %s\n", buffer);

        char command[10], filename[BUFFER_SIZE];
        int client_port;
        if (sscanf(buffer, "%s %s %d", command, filename, &client_port) == 3 && strcmp(command, "GET") == 0) {
            client_data_t *client_data = malloc(sizeof(client_data_t));
            if (client_data == NULL) {
                perror("Memory allocation failed");
                continue;
            }
            strncpy(client_data->filename, filename, BUFFER_SIZE);
            client_data->client_port = client_port;
            client_data->client_addr = client_addr;

            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, client_data);
            pthread_detach(tid);
        } else {
            int response_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (response_sock < 0) {
                perror("Response socket creation failed");
                continue;
            }

            client_addr.sin_port = htons(TCP_PORT);
            if (connect(response_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
                perror("Connect to client failed");
                close(response_sock);
                continue;
            }

            write(response_sock, "INVALID COMMAND", 15);
            close(response_sock);
        }
    }

    close(udp_socket);
    return 0;
}
