#include "http.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

uri_t *parse_uri(const char *uri_in) {
    if(!uri_in) return NULL;

    char *uri_copy = strdup(uri_in);
    char *uri_string = uri_copy;
    char *schema = NULL, *user_pass = NULL, *host = NULL, 
        *path = NULL, *params = NULL, *query = NULL, *frag = NULL, 
        *user = NULL, *pass = NULL, *port = NULL;
    char *ptr;

    // schema
    ptr = strstr(uri_string, "://");
    if(ptr) {
        *ptr = '\0';
        schema = uri_string;
        uri_string = ptr + 3;
    }else{
        free(uri_copy);
        return NULL;
    }

    // frag
    ptr = strchr(uri_string, '#');
    if(ptr) {
        *ptr = '\0';
        frag = ptr + 1;
    }

    // query
    ptr = strchr(uri_string, '?');
    if(ptr) {
        *ptr = '\0';
        query = ptr + 1;
    }

    // params
    ptr = strchr(uri_string, ';');
    if(ptr) {
        *ptr = '\0';
        params = ptr + 1;
    }

    // user_pass
    ptr = strchr(uri_string, '@');
    if(ptr) {
        *ptr = '\0';
        user_pass = uri_string;
        uri_string = ptr + 1;
    }
    
    // path
    ptr = strchr(uri_string, '/');
    if(ptr) {
        *ptr = '\0';
        path = ptr;
    }

    // host and port
    ptr = strchr(uri_string, ':');
    if(ptr) {
        *ptr = '\0';
        host = uri_string;
        port = ptr + 1;
    }else{
        host = uri_string;
    }

    if(user_pass) {
        ptr = user_pass;
        while(*ptr && *ptr != ':') ptr++;
        if(*ptr == ':') {
            *ptr = '\0';
            user = user_pass;
            pass = ptr + 1;
        }else{
            user = user_pass;
        }
    }

    uri_t *uri = calloc(1, sizeof(uri_t));
    uri->schema = strdup(schema);
    if(user) uri->user = strdup(user);
    if(pass) uri->pass = strdup(pass);
    if(host) uri->host = strdup(host);
    if(port) {
        uri->port = atoi(port);
    }else if(strcmp(schema, "http") == 0) {
        uri->port = 80;
    }else if(strcmp(schema, "https") == 0) {
        uri->port = 443;
    }
    if(path) {
        *path = '/';
        uri->path = strdup(path);
    }else{
        // 默认路径
        uri->path = strdup("/");
    }
    if(params) uri->params = strdup(params);
    if(query) uri->query = strdup(query);
    if(frag) uri->frag = strdup(frag);

    return uri;
}

void uri_free(uri_t *uri) {
    if(uri) {
        if(uri->schema) free(uri->schema);
        if(uri->host) free(uri->host);
        if(uri->user) free(uri->user);
        if(uri->pass) free(uri->pass);
        if(uri->path) free(uri->path);
        if(uri->params) free(uri->params);
        if(uri->query) free(uri->query);
        if(uri->frag) free(uri->frag);
        free(uri);
    }
}

void uri_print(uri_t *uri) {
    if(uri) {
        printf("%s://", uri->schema);
        if(uri->user || uri->pass) {
            if(uri->user) printf("%s", uri->user);
            if(uri->pass) printf(":%s", uri->pass);
            printf("@");
        }
        printf("%s:%d", uri->host, uri->port);
        if(uri->path) printf("/%s", uri->path);
        if(uri->params) printf(";%s", uri->params);
        if(uri->query) printf("?%s", uri->query);
        if(uri->frag) printf("#%s", uri->frag);
        printf("\n");
    }else{
        printf("uri is null\n");
    }
}