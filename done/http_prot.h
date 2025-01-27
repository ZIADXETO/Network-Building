/**
 * @file http_prot.h
 * @brief Minimal HTTP protocol services.
 *
 * @author Konstantinos Prasopoulos
 */

#pragma once

#define MAX_HEADERS 40

#define HTTP_HDR_KV_DELIM  ": "
#define HTTP_LINE_DELIM    "\r\n"
#define HTTP_HDR_END_DELIM HTTP_LINE_DELIM HTTP_LINE_DELIM
#define HTTP_PROTOCOL_ID   "HTTP/1.1 "
#define HTTP_OK            "200 OK"
#define HTTP_BAD_REQUEST   "400 Bad Request"

#include <stddef.h>

struct http_string {
    const char *val; // Warning! This is *NOT* null-terminated (thus len field below)
    size_t len;
};

struct http_header {
    struct http_string key;
    struct http_string value;
};

struct http_message {
    struct http_string method;
    struct http_string uri;
    struct http_header headers[MAX_HEADERS];
    size_t num_headers;
    struct http_string body;
};
#ifdef IN_CS202_UNIT_TEST
#define static_unless_test
#else
#define static_unless_test static
#endif

/**
 * @brief Extracts the next token from a string based on a specified delimiter.
 *
 * This function searches for the first occurrence of a delimiter in a provided message string.
 * It sets the 'output' parameter to the portion of the message up to (but not including) the delimiter.
 * If the delimiter is not found, the entire message is considered as a token.
 * This is useful for parsing structured strings where segments are separated by specific characters.
 *
 * @param message The string from which to extract the token.
 * @param delimiter The delimiter used to split the message.
 * @param output A pointer to an http_string structure where the extracted token will be stored.
 *               The 'val' member points to the start of the token and 'len' is set to the token's length.
 *               This structure should be initialized by the caller.
 *
 * @return Returns a pointer to the character following the delimiter in the message, or NULL if the delimiter
 *         is not found, indicating no more tokens are available.
 */

static_unless_test const char* get_next_token(const char* message, const char* delimiter, struct http_string* output);

/**
 * @brief Parses HTTP headers from a given string starting at a specified point.
 *
 * This function iterates through a string containing HTTP headers starting from 'header_start'.
 * It extracts key-value pairs separated by a defined delimiter (HTTP_HDR_KV_DELIM) and stores them
 * in the provided 'output' HTTP message structure. Parsing continues until a NULL character is encountered,
 * the maximum number of headers is reached, or another specified delimiter (HTTP_LINE_DELIM) denotes the end
 * of headers.
 *
 * Each key-value pair is parsed using the get_next_token function, which facilitates the extraction of tokens
 * from the string based on delimiters. The function ensures that headers are not parsed beyond the capacity
 * of the 'output' structure as defined by MAX_HEADERS. If the end delimiter for headers is found, the function
 * returns a pointer to the next character following this delimiter, allowing further processing if necessary.
 *
 * @param header_start Pointer to the start of the headers in the HTTP message.
 * @param output Pointer to the http_message structure where parsed headers will be stored.
 * @return Pointer to the character immediately after the end of headers delimiter, or NULL if headers
 *         end naturally or maximum headers have been read.
 */

static_unless_test const char* http_parse_headers(const char* header_start, struct http_message* output);

/**
 * @brief Checks whether the `message` URI starts with the provided `target_uri`.
 *
 * Returns: 1 if it does, 0 if it does not.
 *
 */
int http_match_uri(const struct http_message *message, const char *target_uri);

/**
 * @brief Accepts a potentially partial TCP stream and parses an HTTP message.
 *
 * Assumes that all characters of stream that are not filled by reading are set to 0.
 *
 * Places the complete HTTP message in out.
 * Also writes the content of header "Content Length" to content_len upon parsing the header in the stream.
 * content_len can be used by the caller to allocate memory to receive the whole HTTP message.
 *
 * Returns:
 *  a negative int if there was an error
 *  0 if the message has not been received completely (partial treatment)
 *  1 if the message was fully received and parsed
 */
int http_parse_message(const char *stream, size_t bytes_received, struct http_message *out, int *content_len);

/**
 * @brief Writes the value of parameter `name` from URL in message to buffer out.
 *
 * Return the length of the value.
 * 0 or negative return values indicate an error.
 */
int http_get_var(const struct http_string* url, const char* name, char* out, size_t out_len);

/**
 * @brief Compare method with verb and return 1 if they are equal, 0 otherwise
 */
int http_match_verb(const struct http_string* method, const char* verb);
