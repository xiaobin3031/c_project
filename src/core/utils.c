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
    size_t len = strlen(prefix);
    return strncmp(str, prefix, len) == 0;
}

int end_with(const char *str, const char *suffix)
{ 
    size_t len1 = strlen(str);
    size_t len2 = strlen(suffix);

    if (len1 < len2) return 0;

    return memcmp(str + len1 - len2, suffix, len2) == 0;
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