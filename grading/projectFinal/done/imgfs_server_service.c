/*
 * @file imgfs_server_services.c
 * @brief ImgFS server part, bridge between HTTP server layer and ImgFS library
 *
 * @author Konstantinos Prasopoulos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // uint16_t
#include <pthread.h>

#include "error.h"
#include "util.h" // atouint16
#include "imgfs.h"
#include "http_net.h"
#include "imgfs_server_service.h"

// Main in-memory structure for imgFS
static struct imgfs_file fs_file;
static uint16_t server_port;
pthread_mutex_t imgfs_mutex;

#define URI_ROOT "/imgfs"
#define DEFAULT_LISTENING_PORT 8000



#define BASE_FILE "index.html"


/**********************************************************************
 * Sends error message.
 ********************************************************************** */
static int reply_error_msg(int connection, int error)
{
#define ERR_MSG_SIZE 256
    char err_msg[ERR_MSG_SIZE]; // enough for any reasonable err_msg
    if (snprintf(err_msg, ERR_MSG_SIZE, "Error: %s\n", ERR_MSG(error)) < 0) {
        fprintf(stderr, "reply_error_msg(): sprintf() failed...\n");
        return ERR_RUNTIME;
    }
    return http_reply(connection, "500 Internal Server Error", "",
                      err_msg, strlen(err_msg));
}

/**********************************************************************
 * Sends 302 OK message.
 ********************************************************************** */
static int reply_302_msg(int connection)
{
    char location[ERR_MSG_SIZE];
    if (snprintf(location, ERR_MSG_SIZE, "Location: http://localhost:%d/" BASE_FILE HTTP_LINE_DELIM,
                 server_port) < 0) {
        fprintf(stderr, "reply_302_msg(): sprintf() failed...\n");
        return ERR_RUNTIME;
    }
    return http_reply(connection, "302 Found", location, "", 0);
}


/**********************************************************************
 * Simple handling of http message. TO BE UPDATED WEEK 13
 ********************************************************************** */

int handle_http_message(struct http_message* msg, int connection)
{
    M_REQUIRE_NON_NULL(msg);
    debug_printf("handle_http_message() on connection %d. URI: %.*s\n",
                 connection,
                 (int) msg->uri.len, msg->uri.val);

    if (http_match_verb(&msg->uri, "/") || http_match_uri(msg, "/index.html")) {
        return http_serve_file(connection, BASE_FILE);
    }

    if (http_match_uri(msg, URI_ROOT "/list")) {
        return handle_list_call(connection);
    } else if (http_match_uri(msg, URI_ROOT "/read")) {
        return handle_read_call(msg, connection);
    } else if (http_match_uri(msg, URI_ROOT "/insert") && http_match_verb(&msg->method, "POST")) {
        return handle_insert_call(msg, connection);
    } else if (http_match_uri(msg, URI_ROOT "/delete")) {
        return handle_delete_call(msg, connection);
    } else {
        return reply_error_msg(connection, ERR_INVALID_COMMAND);
    }
}


/********************************************************************//**
 * Startup function. Create imgFS file and load in-memory structure.
 * Pass the imgFS file name as argv[1] and optionnaly port number as argv[2]
 ********************************************************************** */
int server_startup (int argc, char **argv)
{
    if (argc < 2) return ERR_NOT_ENOUGH_ARGUMENTS;
    const char *imgfs_filename = argv[1];

    int err = do_open(imgfs_filename, "rb+", &fs_file);
    if (err != ERR_NONE) {
        return err;
    }

    if (pthread_mutex_init(&imgfs_mutex, NULL) != 0) {
        fprintf(stderr, "Mutex init has failed\n");
        do_close(&fs_file);
        return ERR_THREADING;
    }

    print_header(&fs_file.header);

    if (argc >= 3) {
        server_port = atouint16(argv[2]);
        if (server_port == 0) {
            server_port = DEFAULT_LISTENING_PORT;
        }
    } else {
        server_port = DEFAULT_LISTENING_PORT;
    }

    err = http_init(server_port, handle_http_message);
    if (err < 0) {
        do_close(&fs_file);
        return ERR_IO;
    }

    printf("ImgFS server started on http://localhost:%d\n", server_port);
    return ERR_NONE;

}

/********************************************************************//**
 * Shutdown function. Free the structures and close the file.
 ********************************************************************** */
void server_shutdown (void)
{
    fprintf(stderr, "Shutting down...\n");
    http_close();
    do_close(&fs_file);
    pthread_mutex_destroy(&imgfs_mutex);
}

/**
 * @brief Handles the 'list' API call, sending a JSON response with file system entries.
 *
 * This function locks the file system, calls the function to list entries, and constructs
 * an HTTP response with the list in JSON format. It handles errors by replying with an
 * appropriate error message and ensures the mutual exclusion on file system access.
 *
 * @param connection The socket connection to send the response to.
 * @return The status of the HTTP response.
 */
int handle_list_call(int connection)
{
    char* json_output = NULL;

    pthread_mutex_lock(&imgfs_mutex);
    int result = do_list(&fs_file, JSON, &json_output);
    pthread_mutex_unlock(&imgfs_mutex);

    if (result != ERR_NONE) {
        if (json_output != NULL) {
            free(json_output);
        }
        return reply_error_msg(connection, result);
    }

    char headers[256];
    snprintf(headers, sizeof(headers),
             "Content-Type: application/json" HTTP_LINE_DELIM,
             strlen(json_output));

    int response_status = http_reply(connection, "200 OK", headers, json_output, strlen(json_output));

    free(json_output);

    return response_status;
}

/**
 * @brief Handles the 'read' API call, sending the requested image data.
 *
 * Extracts resolution and image ID from the URI, reads the corresponding image from the
 * file system, and sends it back to the client. Handles different resolutions and errors
 * such as missing arguments or resolution mismatches.
 *
 * @param msg The HTTP message containing the request.
 * @param connection The socket connection to send the response to.
 * @return The status of the HTTP response.
 */

int handle_read_call(struct http_message* msg, int connection)
{
    char res[10];
    int res_len = http_get_var(&msg->uri, "res", res, sizeof(res));
    if (res_len <= 0) {
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }

    int resolution = resolution_atoi(res);
    if (resolution == -1) {
        return reply_error_msg(connection, ERR_RESOLUTIONS);
    }

    char img_id[MAX_IMG_ID];
    int img_id_len = http_get_var(&msg->uri, "img_id", img_id, sizeof(img_id));
    if (img_id_len <= 0) {
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }

    char* image_buffer = NULL;
    uint32_t image_size = 0;
    pthread_mutex_lock(&imgfs_mutex);
    int result = do_read(img_id, resolution, &image_buffer, &image_size, &fs_file);
    pthread_mutex_unlock(&imgfs_mutex);
    if (result != ERR_NONE) {
        if (image_buffer) {
            free(image_buffer);
        }
        return reply_error_msg(connection, result);
    }

    char headers[256];
    snprintf(headers, sizeof(headers),
             "Content-Type: image/jpeg" HTTP_LINE_DELIM,
             image_size);

    int response_status = http_reply(connection, "200 OK", headers, image_buffer, image_size);

    free(image_buffer);

    return response_status;
}

/**
 * @brief Handles the 'insert' API call, inserting new image data into the file system.
 *
 * Receives image data and a name from the request, inserts it into the file system,
 * and returns a status code. Handles out-of-memory errors and file system full errors
 * by sending appropriate HTTP error messages.
 *
 * @param msg The HTTP message containing the request.
 * @param connection The socket connection to send the response to.
 * @return The status of the HTTP response.
 */

int handle_insert_call(struct http_message* msg, int connection)
{
    char img_name[128];
    int name_len = http_get_var(&msg->uri, "name", img_name, sizeof(img_name));
    if (name_len <= 0) {
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }

    if (msg->body.len == 0) {
        return reply_error_msg(connection, ERR_INVALID_ARGUMENT);
    }
    char* image_buffer = (char*)calloc(msg->body.len, sizeof(char));
    if (image_buffer == NULL) {
        return reply_error_msg(connection, ERR_OUT_OF_MEMORY);
    }
    memcpy(image_buffer, msg->body.val, msg->body.len);

    pthread_mutex_lock(&imgfs_mutex);
    int result = do_insert(image_buffer, msg->body.len, img_name, &fs_file);
    pthread_mutex_unlock(&imgfs_mutex);
    free(image_buffer);
    if (result != ERR_NONE) {
        return reply_error_msg(connection, result);
    }

    return reply_302_msg(connection);
}

/**
 * @brief Handles the 'delete' API call, removing an image from the file system.
 *
 * Retrieves the image ID from the URI and attempts to delete the corresponding image from
 * the file system. Locks the file system during the operation and replies with appropriate
 * status codes based on the outcome.
 *
 * @param msg The HTTP message containing the request.
 * @param connection The socket connection to send the response to.
 * @return The status of the HTTP response.
 */

int handle_delete_call(const struct http_message* msg, int connection)
{
    char img_id[MAX_IMG_ID];
    int img_id_len = http_get_var(&msg->uri, "img_id", img_id, sizeof(img_id));
    if (img_id_len <= 0) {
        return reply_error_msg(connection, ERR_NOT_ENOUGH_ARGUMENTS);
    }

    pthread_mutex_lock(&imgfs_mutex);
    int result = do_delete(img_id, &fs_file);
    pthread_mutex_unlock(&imgfs_mutex);
    if (result != ERR_NONE) {
        return reply_error_msg(connection, result);
    }

    return reply_302_msg(connection);
}

