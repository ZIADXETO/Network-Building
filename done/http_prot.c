/*
 * @file http_net.c
 * @brief HTTP server layer for CS-202 project
 *
 * This file contains the core functions of an HTTP server designed for the CS-202 project. It includes
 * methods for parsing HTTP messages, headers, and matching URIs and HTTP verbs. Additionally, it provides
 * functionality to extract variables from URLs and handle HTTP GET requests.
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
#include <stdbool.h>

#include "http_prot.h"
#include "http_net.h"
#include "socket_layer.h"
#include "error.h"
#include "math.h"

#ifdef IN_CS202_UNIT_TEST
#define static_unless_test
#else
#define static_unless_test static
#endif


static_unless_test const char* get_next_token(const char* message, const char* delimiter, struct http_string* output)
{
    if (!message || !delimiter) return NULL;

    const char* delim_pos = strstr(message, delimiter);
    if (!delim_pos) {
        if (output) {
            output->val = message;
            output->len = strlen(message);
        }
        return NULL;
    }

    if (output) {
        output->val = message;
        output->len = (size_t)(delim_pos - message);
    }

    return delim_pos + strlen(delimiter);
}

static_unless_test const char* http_parse_headers(const char* header_start, struct http_message* output)
{
    const char* current_pos = header_start;
    struct http_string key, value;

    while (current_pos && *current_pos != '\0') {
        current_pos = get_next_token(current_pos, HTTP_HDR_KV_DELIM, &key);

        if (current_pos) {
            current_pos = get_next_token(current_pos, HTTP_LINE_DELIM, &value);
        }

        if (current_pos && output->num_headers < MAX_HEADERS) {
            output->headers[output->num_headers].key = key;
            output->headers[output->num_headers].value = value;
            output->num_headers++;
        }

        if (current_pos && strncmp(current_pos, HTTP_LINE_DELIM, strlen(HTTP_LINE_DELIM)) == 0) {
            return current_pos + strlen(HTTP_LINE_DELIM);
        }
    }

    return NULL;
}

int http_match_uri(const struct http_message *message, const char *target_uri)
{
    M_REQUIRE_NON_NULL(message);
    M_REQUIRE_NON_NULL(target_uri);

    size_t target_len = strlen(target_uri);

    if (message->uri.len < target_len) return 0;

    return strncmp(message->uri.val, target_uri, target_len) == 0;
}

int http_match_verb(const struct http_string* method, const char* verb)
{
    M_REQUIRE_NON_NULL(method);
    M_REQUIRE_NON_NULL(verb);

    size_t verb_len = strlen(verb);

    if (method->len != verb_len) return 0;

    return strncmp(method->val, verb, verb_len) == 0;
}

int http_get_var(const struct http_string* url, const char* name, char* out, size_t out_len)
{
    M_REQUIRE_NON_NULL(url);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(out);

    size_t name_len = strlen(name);
    char *param = (char *)calloc(name_len + 2, sizeof(char));
    if (!param) return ERR_OUT_OF_MEMORY;

    snprintf(param, name_len + 2, "%s=", name);

    const char *start = strstr(url->val, param);
    if (!start || (start >= url->val + url->len)) {
        free(param);
        return 0;
    }

    start += strlen(param);
    const char *end = strchr(start, '&');
    if (!end || (end >= url->val + url->len)) {
        end = url->val + url->len;
    }

    size_t value_len = (unsigned long) (end - start);
    if (value_len >= out_len) {
        free(param);
        return ERR_RUNTIME;
    }

    strncpy(out, start, value_len);
    out[value_len] = '\0';

    free(param);
    return (int)value_len;
}

int http_parse_message(const char *stream, size_t bytes_received, struct http_message *out, int *content_len)
{
    M_REQUIRE_NON_NULL(stream);
    M_REQUIRE_NON_NULL(out);
    M_REQUIRE_NON_NULL(content_len);

    const char *headers_end = strstr(stream, HTTP_HDR_END_DELIM);
    if (!headers_end) return 0;

    *content_len = 0;
    memset(out, 0, sizeof(struct http_message));

    const char *current_pos = stream;
    current_pos = get_next_token(current_pos, " ", &out->method);
    if (!current_pos) return -1;

    current_pos = get_next_token(current_pos, " ", &out->uri);
    if (!current_pos) return -1;

    struct http_string http_version;
    current_pos = get_next_token(current_pos, HTTP_LINE_DELIM, &http_version);
    if (!current_pos || strncmp(http_version.val, HTTP_PROTOCOL_ID, http_version.len) != 0) {
        return -1;
    }

    current_pos = http_parse_headers(current_pos, out);
    if (!current_pos) return -1;

    bool found = false;

    for (size_t i = 0; i < out->num_headers && !found; i++) {
        if (strncmp(out->headers[i].key.val, "Content-Length", out->headers[i].key.len) == 0) {
            char content_length_str[32];
            strncpy(content_length_str, out->headers[i].value.val, out->headers[i].value.len);
            content_length_str[out->headers[i].value.len] = '\0';
            *content_len = atoi(content_length_str);
            found = true;
        }
    }

    if (*content_len > 0) {
        size_t body_offset = (size_t)(headers_end + strlen(HTTP_HDR_END_DELIM) - stream);
        if (bytes_received < body_offset + ((unsigned long)*content_len)) {
            return 0;
        }
        out->body.val = stream + body_offset;
        out->body.len = (size_t)*content_len;
    }

    return 1;
}
