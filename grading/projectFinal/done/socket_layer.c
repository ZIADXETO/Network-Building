#include "socket_layer.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

int tcp_server_init(uint16_t port)
{
    int sockid;
    struct sockaddr_in server_addr;

    sockid = socket(AF_INET, SOCK_STREAM, 0);
    if (sockid < 0) {
        perror("socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockid, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockid);
        return -1;
    }

    if (listen(sockid, 5) < 0) {
        perror("listen");
        close(sockid);
        return -1;
    }

    return sockid;
}

int tcp_accept(int passive_socket)
{
    return accept(passive_socket, NULL, NULL);
}


ssize_t tcp_read(int active_socket, char* buf, size_t buflen)
{
    if (!buf || buflen == 0) {
        return ERR_INVALID_ARGUMENT;
    }
    return recv(active_socket, buf, buflen, 0);
}

ssize_t tcp_send(int active_socket, const char* response, size_t response_len)
{
    if (!response || response_len == 0) {
        return ERR_INVALID_ARGUMENT;
    }
    return send(active_socket, response, response_len, 0);
}

