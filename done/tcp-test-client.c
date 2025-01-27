#include "socket_layer.h"
#include "error.h"
#include "util.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BUFFER_SIZE 2048
#define SIZE_DELIMITER '|'
#define FILE_TERMINATOR "<EOF>"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        return ERR_INVALID_COMMAND;
    }

    uint16_t port = (uint16_t) atoi(argv[1]);
    const char *filepath = argv[2];
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        perror("fopen");
        return ERR_IO;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size >= BUFFER_SIZE) {
        fclose(file);
        return ERR_IO;
    }

    char file_content[BUFFER_SIZE];
    size_t read_bytes = fread(file_content, sizeof(char), (unsigned long) file_size, file);
    if (read_bytes < (unsigned long) file_size) {
        perror("fread");
        fclose(file);
        return ERR_IO;
    }

    file_content[read_bytes] = '\0';
    fclose(file);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return ERR_IO;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connect");
        close(client_socket);
        return ERR_IO;
    }

    printf("Talking to %d\n", port);

    char size_message[BUFFER_SIZE];
    snprintf(size_message, BUFFER_SIZE, "%ld%c", file_size, SIZE_DELIMITER);
    ssize_t sent_bytes = tcp_send(client_socket, size_message, strlen(size_message));
    if (sent_bytes < 0) {
        perror("tcp_send");
        close(client_socket);
        return ERR_IO;
    }

    size_message[sent_bytes - 1] = '\0';

    printf("Sending size %s:\n", size_message);

    char response[BUFFER_SIZE];
    ssize_t received_bytes = tcp_read(client_socket, response, BUFFER_SIZE);
    if (received_bytes < 0) {
        perror("tcp_read");
        close(client_socket);
        return ERR_IO;
    }

    response[received_bytes] = '\0';
    printf("Server responded: \"%s\"\n", response);

    if (strncmp(response, "Large file", MIN(strlen("Large file"), strlen(response)) + 1) == 0) {
        close(client_socket);
        return ERR_NONE;
    }

    printf("Sending %s\n", filepath);
    sent_bytes = tcp_send(client_socket, file_content, read_bytes);
    if (sent_bytes < 0) {
        perror("tcp_send");
        close(client_socket);
        return ERR_IO;
    }

    sent_bytes = tcp_send(client_socket, FILE_TERMINATOR, strlen(FILE_TERMINATOR));
    if (sent_bytes < 0) {
        perror("tcp_send");
        close(client_socket);
        return ERR_IO;
    }

    received_bytes = tcp_read(client_socket, response, BUFFER_SIZE);
    if (received_bytes < 0) {
        perror("tcp_read");
        close(client_socket);
        return ERR_IO;
    }

    response[received_bytes] = '\0';
    printf("%s\n", response);

    close(client_socket);

    printf("Done\n");

    return ERR_NONE;
}
