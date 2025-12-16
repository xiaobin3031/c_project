#include "banner.h"
#include "../../core/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

static int connect_with_timeout(const char *ip, int port, int timeout_ms) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("banner -> socket()\n");
        return -1;
    }

    // set non-blocking
    fcntl(sock, F_SETFL, O_NONBLOCK);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int r = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (r < 0 && errno != EINPROGRESS) {
        printf("banner -> connect()\n");
        close(sock);
        return -1;
    }

    // select
    fd_set w;
    FD_ZERO(&w);
    FD_SET(sock, &w);

    struct timeval tv = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000
    };

    r = select(sock + 1, NULL, &w, NULL, &tv);
    if (r <= 0) {
        printf("banner -> select()\n");
        close(sock);
        return -1;
    }

    int err = 0;
    socklen_t len = sizeof(err);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err != 0) {
        printf("banner -> getsockopt()\n");
        close(sock);
        return -1;
    }

    return sock; // success
}

static banner_t *http_banner(const char *ip, const int port) {
    int sock = connect_with_timeout(ip, port, 1500);
    if (sock < 0) {
        printf("    [!] Could not connect for banner\n");
        return NULL;
    }
    char sendbuf[1024];
    char recvbuf[1024] = {0};
    size_t len = sizeof(sendbuf);
    size_t offset = 0;

    offset += snprintf(sendbuf + offset, len - offset, "HEAD / HTTP/1.0\r\n");
    offset += snprintf(sendbuf + offset, len - offset, "Host: %s:%d\r\n", ip, port);
    offset += snprintf(sendbuf + offset, len - offset, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/142.0.0.0 Safari/537.36\r\n");
    offset += snprintf(sendbuf + offset, len - offset, "Accept: */*\r\n");

    offset += snprintf(sendbuf + offset, len - offset, "\r\n");
    send(sock, sendbuf, strlen(sendbuf), 0);

    usleep(200 * 1000);

    char c;
    int total = 0;
    int bytes;
    len = sizeof(recvbuf);
    while(recv(sock, &c, 1, 0) > 0 && total < len - 1) {
        recvbuf[total++] = c;
        if(total > 4 && end_with(recvbuf, "\r\n\r\n")) {
            total -= 2;
            break;
        }
    }
    recvbuf[total] = 0;
    banner_t *banner;
    if (total > 0) {
        banner = malloc(sizeof(banner_t));
        banner->type = BANNER_HTTP;
        banner->banner = strdup(recvbuf);
    }

    return banner;
}

static char *banner_type_string(banner_type type) {
    switch (type) {
        case BANNER_DNS:
            return "DNS";
        case BANNER_FTP:
            return "FTP";
        case BANNER_HTTP:
            return "HTTP";
        case BANNER_HTTPS:
            return "HTTPS";
        case BANNER_ICMP:
            return "ICMP";
        case BANNER_IMAP:
            return "IMAP";
        case BANNER_MYSQL:
            return "MYSQL";
        case BANNER_NTP:
            return "NTP";
        case BANNER_POP3:
            return "POP3";
        case BANNER_RDP:
            return "RDP";
        case BANNER_REDIS:
            return "REDIS";
        case BANNER_SMB:
            return "SMB";
        case BANNER_SMTP:
            return "SMTP";
        case BANNER_SSH:
            return "SSH";
        case BANNER_TELNET:
            return "TELNET";
        default:
            return "UNKNOWN";
    }
}

static void detect_service(banner_t *banner, const finger_rule_db *db) { 
    // todo detect_service
    service_info *service = malloc(sizeof(service_info));
    if(db && db->rule_count > 0) {
        for(int i = 0; i < db->rule_count; i++) {
            finger_rule *rule = db->rules[i];
            int matched = 1;
            for(int j = 0; j < rule->match_count; j++) {
                if(!strstr(banner->banner, rule->match_strings[j])) {
                    matched = 0;
                    break;
                }
            }
            if(matched) {
                strncpy(service->service, rule->service, sizeof(service->service));
                strncpy(service->protocol, rule->protocol, sizeof(service->protocol));
                banner->service = service;
                return;
            }
        }
    }
    strcpy(service->service, "unknown");
    strcpy(service->protocol, "unknown");
    banner->service = service;
}

banner_t *grab_banner(const char *ip, int port, const finger_rule_db *db) {
    int sock = connect_with_timeout(ip, port, 800);
    if (sock < 0) {
        printf("    [!] Could not connect for banner\n");
        return NULL;
    }

    char sendbuf[256];
    char recvbuf[BANNER_MAX] = {0};

    sprintf(sendbuf, "HELLO\r\n");
    send(sock, sendbuf, strlen(sendbuf), 0);

    usleep(200 * 1000);

    banner_t *banner;
    // read response
    int bytes = recv(sock, recvbuf, sizeof(recvbuf)-1, 0);
    if (bytes > 0) {
        recvbuf[bytes] = 0;
        if(start_with(recvbuf, "HTTP/")) {
            banner = http_banner(ip, port);
        }else if(start_with(recvbuf, "SSH-")) {
            banner = malloc(sizeof(banner_t));
            banner->type = BANNER_SSH;
            banner->banner = strdup(recvbuf);
        }
    } else {
        printf("    [!] No banner received.\n");
    }
    if(banner) {
        banner->port = port;
        if(db) detect_service(banner, db);
    }

    close(sock);
    return banner;
}

finger_rule_db *finger_rule_db_load(const char *filepath) {
    // todo load finger rules 目录在 ../data/finger_rule
    FILE *file = fopen(filepath, "r");
    if(!file) {
        printf("[!] Failed to open finger rule file: %s\n", filepath);
        return NULL;
    }

    char line[512];
    int count = 0;

    finger_rule_db *db = malloc(sizeof(finger_rule_db));
    int max_match = 5, max_all_rules = 1000, rule_count = 0;
    char *rules[max_match];
    finger_rule *all_rules[max_all_rules];
    while(fgets(line, sizeof(line), file)) {
        if(line[0] == '#' || line[0] == '\0') continue;

        char *service = strtok(line, "|");
        char *protocol = strtok(NULL, "|");
        char *matches = strtok(NULL, "|");
        if(!service || !protocol || !matches) continue;
        char *version = strtok(NULL, "|");

        finger_rule *rule = malloc(sizeof(finger_rule));
        rule->service = strdup(service);
        rule->protocol = strdup(protocol);
        if(version) rule->version_regexp = strdup(version);

        int count = 0;
        if(matches) {
            char *token = strtok(matches, ",");
            while(token && count < max_match) {
                rules[count++] = token;
                token = strtok(NULL, ",");
            }
        }
        rule->match_count = count;
        if(count > 0) {
            rule->match_strings = malloc(sizeof(char *) * count);
            for(int i=0;i<count;i++) {
                rule->match_strings[i] = strdup(rules[i]);
            }
        }
        all_rules[rule_count++] = rule;
        if(rule_count >= max_all_rules) {
            printf("Maximum number[%d] of fingerprint rules reached.\n", max_all_rules);
        }
    }

    db->rule_count = rule_count;
    if(rule_count > 0) {
        db->rules = malloc(sizeof(finger_rule *) * rule_count);
        for(int i=0;i<rule_count;i++) {
            db->rules[i] = all_rules[i];
        }
    }

    return db;
}

void banner_free(banner_t *banner) {
    if(banner) {
        if(banner->banner) free(banner->banner);
        if(banner->service) free(banner->service);
        free(banner);
    }
}

void finger_rule_db_free(finger_rule_db *db) {
    if(db) {
        if(db->rules) {
            int i;
            for(i = 0; i < db->rule_count; i++) {
                finger_rule_free(db->rules[i]);
            }
            free(db->rules);
        }
        free(db);
    }
}

void finger_rule_db_print(finger_rule_db *db) {
    if(db) {
        int i;
        for(i = 0; i < db->rule_count; i++) {
            finger_rule_print(db->rules[i]);
        }
    }
}

void finger_rule_print(finger_rule *rule) { 
    if(rule) { 
        printf("finger fule: %s %s\n", rule->protocol, rule->service);
        if(rule->match_count > 0) {
            printf("matches:\n");
            for(int i=0;i<rule->match_count;i++) {
                printf("  %s\n", rule->match_strings[i]);
            }
        }
        printf("version regexp: %s\n", rule->version_regexp);
    }
}

void finger_rule_free(finger_rule *rule) {
    if(rule) {
        if(rule->match_strings) {
            int i;
            for(i = 0; i < rule->match_count; i++) {
                free(rule->match_strings[i]);
            }
            free(rule->match_strings);
        }
        if(rule->service) free(rule->service);
        if(rule->protocol) free(rule->protocol);
        if(rule->version_regexp) free(rule->version_regexp);
        free(rule);
    }
}

void banner_print(banner_t *banner) {
    if(banner) {
        printf("    [+] Port: %d, Banner: %s\n--------\n%s\n--------\n", 
            banner->port, banner_type_string(banner->type), banner->banner);
        service_info_print(banner->service);
    }
}

void service_info_print(service_info *info) {
    if(info) {
        printf("    [+] Protocol: %s, Service: %s, Version: %s\n", 
            info->protocol, info->service, info->version);
    }
}