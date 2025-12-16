#include "syn_scan.h"
#include "../../core/utils.h"
#include "banner.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>

static char target_ip[16];
static int open_ports[65536];
static int total_ports = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int try_read(int sock, int timeout_ms) {
    fd_set writefds, exceptfds;
    struct timeval tv;
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(sock, &writefds);
    FD_SET(sock, &exceptfds);

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int sock_err = 0;
    int res = select(sock + 1, NULL, &writefds, &exceptfds, &tv);
    if (res > 0) {
        if (FD_ISSET(sock, &exceptfds) || FD_ISSET(sock, &writefds)) {
            socklen_t len = sizeof(sock_err);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &sock_err, &len) < 0) {
                return errno;
            }
            return sock_err;
        }
    }
    return ETIMEDOUT;
}

static void *scan_port(void *arg) {
    int port = *(int*)arg;
    free(arg);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        printf("socket error, port: %d\n", port);
        return NULL;
    }
    // 设置非阻塞 (preserve existing flags)
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags != -1) fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, target_ip, &addr.sin_addr);
    
    int err = connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    if (err < 0 && errno != EINPROGRESS) {
        printf("[x] connect failed on port %d, errno=%d\n", port, errno);
    } else {
        /* wait for the socket to become writable or excepted
         * use a single, shorter timeout to reduce per-port latency */
        int tries = 2;
        int timeout_ms = 800; /* per-try timeout in ms */
        int sock_err = ETIMEDOUT;
        for (int t = 0; t < tries; t++) {
            sock_err = try_read(sock, timeout_ms);
            if (sock_err != ETIMEDOUT) break;
        }
        err = sock_err;
    }
    if (err == 0) {
        printf("[+] TCP port %d open\n", port);
        pthread_mutex_lock(&mutex);
        open_ports[total_ports++] = port;
        pthread_mutex_unlock(&mutex);
    }

    close(sock);
    return NULL;
}

void syn_scan(cli_args *args)
{
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    printf("[*] SYN scan on %s ports %d-%d\n",
           args->target, args->port_start, args->port_end);

    strcpy(target_ip, args->target);
    pthread_t threads[MAX_THREADS];
    int thread_count = 0;
    for( int port = args->port_start; port <= args->port_end; port++) {
        int *p = malloc(sizeof(int));
        *p = port;

        pthread_create(&threads[thread_count++], NULL, scan_port, p);
        if(thread_count >= MAX_THREADS) {
            for(int i=0;i<thread_count;i++) pthread_join(threads[i], NULL); 
            thread_count = 0;
        }
    }
    for(int i=0;i<thread_count;i++) pthread_join(threads[i], NULL); 

    printf("[*] SYN scan done.\n--------------\n");
    for(int i=0;i<total_ports;i++) {
        printf("[+] %d open\n", open_ports[i]);
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
    printf("[*] Total scan time: %.3f seconds\n", elapsed);

    finger_rule_db *db = finger_rule_db_load("../src/data/finger_rule");
    banner_t *banners[total_ports];
    for(int i=0;i<total_ports;i++) {
        banners[i] = grab_banner(args->target, open_ports[i], db);
        banner_print(banners[i]);
    }

    for(int i=0;i<total_ports;i++) {
        banner_free(banners[i]);
    }
    finger_rule_db_free(db);
}
