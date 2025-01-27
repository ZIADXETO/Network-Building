#include "socket_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 2048
#define FILE_TERMINATOR "<EOF>"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        return EXIT_FAILURE;
    }

    uint16_t port_number = (uint16_t) atoi(argv[1]);

    int server_socket = tcp_server_init(port_number);
    if (server_socket < 0) {
        return EXIT_FAILURE;
    }

    printf("Server started on port %d\n", port_number);

    while (1) {
        printf("Waiting for a size...\n");

        int client_socket = tcp_accept(server_socket);
        if (client_socket < 0) {
            continue;
        }

        char size_buffer[BUFFER_SIZE];
        ssize_t received_bytes = tcp_read(client_socket, size_buffer, BUFFER_SIZE);
        if (received_bytes < 0) {
            close(client_socket);
            continue;
        }

        size_buffer[received_bytes - 1] = '\0';  // Null-terminate the received string

        printf("Received a size: %s --> ", size_buffer);

        // Extract size from the message
        size_t file_size = (size_t) atoi(size_buffer);

        // Acknowledge the size message
        if (file_size < BUFFER_SIZE/2) {
            tcp_send(client_socket, "Small file", strlen("Small file"));
            printf("accepted\n");
        } else {
            tcp_send(client_socket, "Large file", strlen("Large file"));
            printf("rejected\n");
            close(client_socket);
            continue;
        }

        printf("About to receive file of %zu bytes\n", file_size);

        // Receive the file content
        char file_buffer[BUFFER_SIZE];
        received_bytes = tcp_read(client_socket, file_buffer, file_size);
        if (received_bytes < 0) {
            close(client_socket);
            continue;
        }

        file_buffer[received_bytes] = '\0';  // Null-terminate the received file content
        printf("Received a file:\n%s\n", file_buffer);

        // Acknowledge the file receipt
        tcp_send(client_socket, "Accepted", strlen("Accepted"));

        close(client_socket);
    }

    close(server_socket);
    return EXIT_SUCCESS;
}
