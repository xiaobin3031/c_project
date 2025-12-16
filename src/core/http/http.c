#include "http.h"
#include "../list/arraylist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

static int is_crlf(char *buffer) {
    return *buffer == '\r' && *(buffer+1) == '\n';
}

static http_res_t *read_res(int sock) {
    char buffer[1024];
    char ch;
    size_t offset = 0;
    int is_head_expand = 0;
    while(recv(sock, &ch, sizeof(char), 0)) {
        buffer[offset] = ch;
        if(ch == ' ' || ch == '\t') {
            if(offset > 2 && is_crlf(buffer + offset-2)) {
                // 首部延续行
                is_head_expand = 1;
                offset -= 2;
                buffer[offset] = ' ';
            }
        }else if(is_head_expand) {
            is_head_expand = 0;
        }
        if(offset > 4 && is_crlf(buffer+offset-3) && is_crlf(buffer +offset-1)){
            break;
        }
        offset++;
    }
    buffer[--offset] = '\0';

    // res line and heads
    // res line
    char *line = strtok(buffer, CRLF);
    char *ptr;
    while(*line != '/') line++;
    line++;
    ptr = line;
    http_res_t *res = malloc(sizeof(http_res_t));
    while(*ptr != ' ') ptr++;
    *ptr = '\0';
    res->version = strdup(line);
    line = ptr+1;
    ptr = line;
    while(*ptr != ' ') ptr++;
    *ptr = '\0';
    res->status_code = atoi(line);
    res->reason = strdup(ptr + 1);

    // heads
    http_head_t *res_head = malloc(sizeof(http_head_t));
    res_head->keys = arraylist_new(10);
    res_head->values = arraylist_new(10);
    size_t content_length = 0;
    while((line = strtok(NULL, CRLF))) {
        ptr = line;
        while(*ptr != ':') ptr++;
        *ptr = '\0';
        ptr++;
        while(*ptr == ' ') ptr++;
        if(strcmp(line, "Content-Type") == 0) {
            res_head->content_type = strdup(ptr);
        }else if(strcmp(line, "Transfer-Encoding") == 0 && strcmp(ptr, "chunked")) {
            res_head->chunked = 1;
        } else if(strcmp(line, "Content-Length") == 0) {
            content_length = atol(ptr);
        } else {
            arraylist_add(res_head->keys, strdup(line));
            arraylist_add(res_head->values, strdup(ptr));
        }
    }

    // body
    if(res_head->chunked) {
        offset = 0;
        size_t capacity = 1000, size = 0;

        char *body = malloc(sizeof(char) * capacity);
        while(recv(sock, &ch, sizeof(char), 0)) {
            buffer[offset] = ch;
            if(offset > 1 && is_crlf(buffer +offset - 1)) {
                buffer[offset - 1] = '\0';
                size_t tmp_size = atol(buffer);
                if(tmp_size == 0) {
                    break;
                }
                if(tmp_size > capacity - size) {
                    capacity += 1000;
                    body = realloc(body, capacity);
                }
                recv(sock, body + size, tmp_size, 0);
                // 读取两个结束符
                recv(sock, &ch, sizeof(char), 0);
                recv(sock, &ch, sizeof(char), 0);
                size += tmp_size;
            }
        }
        // 结束了，也读取两个结束符
        recv(sock, &ch, sizeof(char), 0);
        recv(sock, &ch, sizeof(char), 0);
        res->body = malloc(sizeof(char) * (size + 1));
        memcpy(res->body, body, size);
        res->body[size] = '\0';
        res->body_size = size;
        free(body);
    } else if(content_length > 0) {
        res->body = malloc(sizeof(char) * (content_length + 1));
        if(!recv(sock, res->body, content_length, 0)) {
            http_res_free(res);
            perror("read body error");
            return NULL;
        }
        res->body_size = content_length;
        res->body[content_length] = '\0';
    }

    res->heads = res_head;

    return res;
}

char *http_method_string(http_method_e method) {
    switch(method) {
        case  HTTP_METHOD_GET:
            return "GET";
        case HTTP_METHOD_HEAD:
            return "HEAD";
        case HTTP_METHOD_OPTION:
            return "OPTION";
        case HTTP_METHOD_POST:
            return "POST";
        case HTTP_METHOD_PUT:
            return "PUT";
        default:
            return NULL;
    }
}

size_t write_http_head(char *request, http_t *http) {
    size_t offset = 0;
    offset += sprintf(request + offset, "%s %s HTTP/1.1\r\n", http_method_string(http->method), http->uri->path);
    // offset += sprintf(request + offset, "Host: %s\r\n", http->uri->host);
    offset += sprintf(request + offset, "Connection: keep-alive\r\n");
    if(http->body_size > 0) offset += sprintf(request + offset, "Content-Length: %ld\r\n", http->body_size);
    http_head_t *heads = http->heads;
    if(heads) {
        if(heads->content_type) offset += sprintf(request + offset, "Content-Type: %s\r\n", heads->content_type);
        if(heads->accept) {
            offset += sprintf(request + offset, "Accept: %s\r\n", heads->accept);
        }else{
            offset += sprintf(request + offset, "Accept: */*\r\n");
        }
        if(heads->user_agent) {
            offset += sprintf(request + offset, "User-Agent: %s\r\n", heads->user_agent);
        }else{
            offset += sprintf(request + offset, "User-Agent: %s\r\n", USER_AGENT_CHROM);
        }

        if(heads->keys && heads->values) {
            for(int i=0;i<heads->keys->size;i++) {
                char *key = (char*) arraylist_get(heads->keys, i);
                char *val = (char*) arraylist_get(heads->values, i);
                if(key && val) {
                    offset += sprintf(request + offset, "%s: %s\r\n", key, val);
                }
            }
        }
    }else{
        offset += sprintf(request + offset, "User-Agent: %s\r\n", USER_AGENT_CHROM);
        offset += sprintf(request + offset, "Accept: */*\r\n");
    }
    offset += sprintf(request + offset, "\r\n");

    return offset;
}

/**
 * 发送http请求
 */
http_res_t *send_http(http_t *http) {
    if(!http || !http->uri) {
        perror("http 或 http->uri 为空");
        return NULL;
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        perror("socket error");
        return NULL;
    }
    size_t body_size = http->body_size;
    uri_t *uri = http->uri;
    char request[1024 + body_size];
    size_t offset = write_http_head(request, http);
    request[offset] = '\0';
    printf("HTTP Request Head:\n%s", request);
    // 复制报文体
    if(http->body) {
        memcpy(request + offset, http->body, http->body_size);
        offset += body_size;
    }

    request[offset] = '\0';

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(uri->port);
    inet_pton(AF_INET, uri->host, &server.sin_addr);

    int err = connect(sock, (struct sockaddr *)&server, sizeof(server));
    if(err < 0) {
        perror("connect error");
        return NULL;
    }

    err = send(sock, request, offset, 0);
    if(err < 0) {
        perror("send error");
        return NULL;
    }

    // 读取返回值
    return read_res(sock);
}


void http_head_free(http_head_t *http_head) {
    if(http_head) {
        if(http_head->content_type) free(http_head->content_type);
        if(http_head->accept) free(http_head->accept);
        if(http_head->user_agent) free(http_head->user_agent);
        arraylist_free(http_head->keys, free);
        arraylist_free(http_head->values, free);
        free(http_head);
    }
}

void http_head_print(http_head_t *http_head) {
    if(http_head) {
        printf("http head: \n");
        if(http_head->content_type) printf("Content-Type: %s\n", http_head->content_type);
        printf("body chunked: %c\n", http_head->chunked ? 'Y' : 'N');
    }
}

void http_free(http_t *http) {
    if(http) {
        uri_free(http->uri);
        http_head_free(http->heads);
        if(http->body) free(http->body);

        free(http);
    }
}

void http_res_free(http_res_t *res) {
    if(res) {
        if(res->version) free(res->version);
        if(res->reason) free(res->reason);
        http_head_free(res->heads);
        if(res->body) free(res->body);

        free(res);
    }
}

void http_res_print(http_res_t *res) {
    if(res) {
        printf("http res: \n");
        printf("HTTP");
        if(res->version) printf("/%s", res->version);
        printf(" %d", res->status_code);
        if(res->reason) printf(" %s", res->reason);
        printf("\n");
        http_head_print(res->heads);
        printf("body size: %ld\n", res->body_size);
        // if(res->body_size > 0)
            // printf("%s\n", res->body);
    }else{
        printf("http res is null\n");
    }
}