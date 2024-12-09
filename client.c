#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define UDP_PORT 5555
#define BUFFER_SIZE 1024
#define RESPONSE_PORT 6666

void send_udp_request(const char *server_ip, const char *filename, int data_port) {
    int udp_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    // Format and send the request
    snprintf(buffer, BUFFER_SIZE, "GET %s %d", filename, data_port);
    if (sendto(udp_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to send UDP packet");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }
    printf("Sent UDP request: %s\n", buffer);
    close(udp_socket);
}

void receive_file(int data_port) {
    int tcp_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    FILE *file;

    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    listen(tcp_socket, 1);
    printf("Waiting for file on port %d...\n", data_port);

    int client_sock = accept(tcp_socket, NULL, NULL);
    if (client_sock < 0) {
        perror("Accept failed");
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    file = fopen("received_file.txt", "w");
    if (!file) {
        perror("Failed to open file");
        close(client_sock);
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    printf("File received and saved as 'received_file.txt'\n");
    fclose(file);
    close(client_sock);
    close(tcp_socket);
}

void receive_response() {
    int tcp_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(RESPONSE_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    listen(tcp_socket, 1);
    printf("Waiting for server response on port %d...\n", RESPONSE_PORT);

    int client_sock = accept(tcp_socket, NULL, NULL);
    if (client_sock < 0) {
        perror("Accept failed");
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Server response: %s\n", buffer);
    }

    close(client_sock);
    close(tcp_socket);
}

int main() {
    const char *server_ip = "127.0.0.1"; // Địa chỉ server
    const char *filename = "example.txt"; // Tên file muốn yêu cầu
    int data_port = 12345; // Cổng TCP để nhận file

    // Gửi yêu cầu UDP
    send_udp_request(server_ip, filename, data_port);

    // Nhận file qua TCP
    receive_file(data_port);

    // Nhận phản hồi từ server
    receive_response();

    return 0;
}
