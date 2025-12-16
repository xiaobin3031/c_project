#pragma once

#include "../list/arraylist.h"

#define CRLF "\r\n"
#define USER_AGENT_CHROM "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/142.0.0.0 Safari/537.36"

typedef enum {
    HTTP_METHOD_POST,
    HTTP_METHOD_GET,
    HTTP_METHOD_OPTION,
    HTTP_METHOD_PUT,
    HTTP_METHOD_HEAD
} http_method_e;

typedef struct {
    char *schema;
    char *user;
    char *pass;
    char *host;
    int port;
    char *path;
    char *params;
    char *query;
    char *frag;
} uri_t;

typedef struct {
    char *content_type;
    char *accept;
    char *user_agent;
    int chunked;

    arraylist *keys;
    arraylist *values;

} http_head_t;

typedef struct {
    http_method_e method;        // HTTP method (GET, POST, etc.)
    uri_t *uri;
    http_head_t *heads;  // Request headers
    char *body;          // Request body (for POST, PUT, etc.)
    size_t body_size;
} http_t;

typedef struct {
    int status_code;
    char *version;
    char *reason;
    http_head_t *heads;
    char *body;
    size_t body_size;
} http_res_t;


uri_t *parse_uri(const char *uri_in);

// http 
char *http_method_string(http_method_e method);
size_t write_http_head(char *buffer, http_t *http);
http_res_t *send_http(http_t *http);


void uri_free(uri_t *uri);
void uri_print(uri_t *uri);

void http_head_free(http_head_t *http_head);
void http_head_print(http_head_t *http_head);

void http_free(http_t *http);

void http_res_free(http_res_t *res);
void http_res_print(http_res_t *res);