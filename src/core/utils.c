#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

// UDP connect trick — silently get local IP
char* get_local_ip() {
    static char ip[INET_ADDRSTRLEN];

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return NULL;

    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_port = htons(53);                  // DNS端口，无所谓
    inet_pton(AF_INET, "8.8.8.8", &dst.sin_addr);  // 一个可路由IP即可

    // 不会发包，但会触发系统选择本地出口IP
    connect(sock, (struct sockaddr*)&dst, sizeof(dst));

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    getsockname(sock, (struct sockaddr*)&name, &namelen);

    inet_ntop(AF_INET, &name.sin_addr, ip, sizeof(ip));
    close(sock);

    return ip;
}

int start_with(const char *str, const char *prefix)
{
    while(*prefix) {
        if(*str++ != *prefix++) return 0;
    }
    return 1;
}

int end_with(const char *str, const char *suffix)
{ 
    const char *ptr = str + strlen(str) - strlen(suffix);
    while(*suffix) {
        if(*ptr++ != *suffix++) return 0;
    }
    return 1;
}

int is_integer(const char *str)
{
    while(*str) {
        if(!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

char *fgetline(FILE *fp) {
    size_t size = 128;
    size_t len = 0;
    char *buffer = malloc(size);
    if(!buffer) return NULL;

    while(1) {
        if(fgets(buffer + len, size - len, fp) == NULL) {
            if(len == 0) {
                free(buffer);
                return NULL;
            }
            break;
        }
        len += strlen(buffer + len);
        if(len > 0 && buffer[len - 1] == '\n') {
            break;
        }
        // need more space
        size *= 2;
        buffer = realloc(buffer, size);
        if(!buffer) return NULL;
    }
    // remove trailing newline
    if(len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    return buffer;
}