/*
 * @file http_net.c
 * @brief HTTP server layer for CS-202 project
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>

#include "http_prot.h"
#include "http_net.h"
#include "socket_layer.h"
#include "error.h"
#include "math.h"

static int passive_socket = -1;
static EventCallback cb;

#define MK_OUR_ERR(X) \
static int our_ ## X = X

MK_OUR_ERR(ERR_NONE);
MK_OUR_ERR(ERR_INVALID_ARGUMENT);
MK_OUR_ERR(ERR_OUT_OF_MEMORY);
MK_OUR_ERR(ERR_IO);

/**
 * @fn handle_connection
 * @brief Handles a single client connection in a separate thread.
 *
 * This function is designed to manage a client connection by reading HTTP requests, parsing them,
 * and responding accordingly. It handles signal blocking for graceful shutdowns and errors. Memory for
 * the connection socket is managed, and buffer extensions are performed as needed based on parsed content lengths.
 * If an error occurs during reading, parsing, or memory allocation, the function cleans up and returns
 * an appropriate error code. Upon successful handling, it cleans up resources and returns a success code.
 *
 * Steps involved:
 * 1. Block signals SIGINT and SIGTERM to avoid premature termination.
 * 2. Allocate and manage a dynamic buffer for reading and processing HTTP messages.
 * 3. Continuously read from the socket until there's no more data or an error occurs.
 * 4. Parse the HTTP message, handle dynamic buffer resizing based on content length, and process the message.
 * 5. After processing, reset the buffer and prepare for the next message or terminate the connection.
 *
 * @param arg A pointer to an integer socket descriptor dynamically allocated before thread creation.
 * @return Returns a pointer to a predefined error code or success code.
 */

static void *handle_connection(void *arg)
{
    if (arg == NULL) return &our_ERR_INVALID_ARGUMENT;

    /* Convert void* to int*, dereference to get socket descriptor, and free memory */
    int client_sock = *((int *)arg);
    free(arg);

    /* Set up signal masking for thread-safe shutdown */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    /* Allocate buffer for reading HTTP headers, check for allocation failure */
    char *buffer = calloc(1, MAX_HEADER_SIZE);
    if (buffer == NULL) {
        perror("calloc");
        close(client_sock);
        return &our_ERR_OUT_OF_MEMORY;
    }

    /* Reading loop: read from socket, parse messages, handle requests, and cleanup */
    ssize_t bytes_read;
    size_t total_bytes_read = 0;
    int content_len = 0;
    struct http_message message;
    int parse_result;
    int buffer_size = MAX_HEADER_SIZE;

    bool should_continue = true;
    while (should_continue) {
        bytes_read = tcp_read(client_sock, buffer + total_bytes_read, ((unsigned long)buffer_size) - total_bytes_read);
        if (bytes_read < 0) {
            perror("tcp_read");
            free(buffer);
            close(client_sock);
            return &our_ERR_IO;
        } else if (bytes_read == 0) {
            should_continue = false;
        }

        if (should_continue) {
            total_bytes_read += (unsigned long)bytes_read;

            parse_result = http_parse_message(buffer, total_bytes_read, &message, &content_len);
            if (parse_result < 0) {
                free(buffer);
                close(client_sock);
                return &our_ERR_INVALID_ARGUMENT;
            }

            /* Handle buffer resizing based on content length */
            if (parse_result == 0 && content_len > 0 && total_bytes_read < (size_t)(content_len + MAX_HEADER_SIZE)) {
                buffer_size = content_len + MAX_HEADER_SIZE;
                char *new_buf = realloc(buffer, (unsigned long)buffer_size);
                if (new_buf == NULL) {
                    perror("realloc");
                    free(buffer);
                    close(client_sock);
                    return &our_ERR_OUT_OF_MEMORY;
                }
                buffer = new_buf;
            }

            /* Process the complete message */
            if (parse_result == 1) {
                cb(&message, client_sock);

                memset(buffer, 0, (unsigned long)buffer_size);
                total_bytes_read = 0;
                content_len = 0;
                parse_result = 0;
            }

            /* Check if buffer is fully consumed */
            if (total_bytes_read >= (size_t)buffer_size) {
                free(buffer);
                close(client_sock);
                return &our_ERR_IO;
            }
        }
    }

    free(buffer);
    close(client_sock);
    return &our_ERR_NONE;
}


/*******************************************************************
 * Init connection
 */
int http_init(uint16_t port, EventCallback callback)
{
    passive_socket = tcp_server_init(port);
    cb = callback;
    return passive_socket;
}

/*******************************************************************
 * Close connection
 */
void http_close(void)
{
    if (passive_socket > 0) {
        if (close(passive_socket) == -1)
            perror("close");
        else
            passive_socket = -1;
    }
}

/*******************************************************************
 * Receive content
 */
int http_receive(void)
{
    int *active_socket = calloc(1, sizeof(int));
    if (!active_socket) {
        perror("calloc");
        return our_ERR_OUT_OF_MEMORY;
    }

    *active_socket = tcp_accept(passive_socket);
    if (*active_socket == -1) {
        perror("tcp_accept");
        free(active_socket);
        return our_ERR_IO;
    }

    pthread_t thread;
    pthread_attr_t attr;

    if (pthread_attr_init(&attr) != 0) {
        perror("pthread_attr_init");
        free(active_socket);
        return ERR_THREADING;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("pthread_attr_setdetachstate");
        free(active_socket);
        pthread_attr_destroy(&attr);
        return ERR_THREADING;
    }

    if (pthread_create(&thread, &attr, handle_connection, active_socket) != 0) {
        perror("pthread_create");
        free(active_socket);
        pthread_attr_destroy(&attr);
        return ERR_THREADING;
    }

    pthread_attr_destroy(&attr);
    return ERR_NONE;
}

/*******************************************************************
 * Serve a file content over HTTP
 */
int http_serve_file(int connection, const char* filename)
{
    M_REQUIRE_NON_NULL(filename);

    // open file
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "http_serve_file(): Failed to open file \"%s\"\n", filename);
        return http_reply(connection, "404 Not Found", "", "", 0);
    }

    // get its size
    fseek(file, 0, SEEK_END);
    const long pos = ftell(file);
    if (pos < 0) {
        fprintf(stderr, "http_serve_file(): Failed to tell file size of \"%s\"\n",
                filename);
        fclose(file);
        return ERR_IO;
    }
    rewind(file);
    const size_t file_size = (size_t) pos;

    // read file content
    char* const buffer = calloc(file_size + 1, 1);
    if (buffer == NULL) {
        fprintf(stderr, "http_serve_file(): Failed to allocate memory to serve \"%s\"\n", filename);
        fclose(file);
        return ERR_IO;
    }

    const size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "http_serve_file(): Failed to read \"%s\"\n", filename);
        fclose(file);
        return ERR_IO;
    }

    // send the file
    const int  ret = http_reply(connection, HTTP_OK,
                                "Content-Type: text/html; charset=utf-8" HTTP_LINE_DELIM,
                                buffer, file_size);

    // garbage collecting
    fclose(file);
    free(buffer);
    return ret;
}


/*******************************************************************
 * Create and send HTTP reply
 */
int http_reply(int connection, const char* status, const char* headers, const char *body, size_t body_len)
{
    M_REQUIRE_NON_NULL(status);
    M_REQUIRE_NON_NULL(headers);
    if (body == NULL && body_len != 0) {
        return ERR_INVALID_ARGUMENT;
    }

    double log_result = log10((double)body_len);
    size_t x = (body_len == 0) ? 0 : (size_t)log_result;
    size_t total_size = strlen(HTTP_PROTOCOL_ID) + 1 + strlen(status) + strlen(HTTP_LINE_DELIM) +
                        strlen(headers) + strlen("Content-Length: ") + strlen(HTTP_HDR_END_DELIM) + x;

    char *response = (char *)calloc(total_size + body_len + 1, sizeof(char));
    if (response == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    snprintf(response, total_size + 1, "%s%s%s%sContent-Length: %zu%s", HTTP_PROTOCOL_ID, status, HTTP_LINE_DELIM,
             headers, body_len, HTTP_HDR_END_DELIM);

    if (body != NULL && body_len > 0) {
        memcpy(response + total_size, body, body_len);
    }

    ssize_t sent_bytes = tcp_send(connection, response, total_size + body_len);
    free(response);

    if (sent_bytes < 0) return ERR_IO;

    return ERR_NONE;
}
