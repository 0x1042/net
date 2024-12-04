#include "relay.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "log.h"

void show_usage(const char * name) {
    printf("Usage: %s <option> [value]\n", name);
    printf("Options:\n");
    printf("  -p, --port          listen port\n");
    printf("  -b, --bufsize       buffer size\n");
    printf("  -l, --backlog       backlog size\n");
    printf("  -v, --verbos        verbos log\n");
    printf("  -f, --fastopen      enable fastopen\n");
    printf("  -r, --reuseaddr     enable reuse address\n");
    printf("  -n, --nodelay       enable tcp nodelay\n");
    printf("  -w, --worker        worker number\n");
    printf("  -h, --help          print help\n");
}

#define STR_CMP(s, l) (strcmp(arg, #s) == 0 || strcmp(arg, #l) == 0)

#define ARGV_TO_INT(s, l, d) \
    if (STR_CMP(s, l) && i + 1 < argc) { \
        opt->d = atoi(argv[i + 1]); \
    }

void parse_command_line(int argc, char ** argv, option_t * opt) {
    if (argc < 2) {
        show_usage(argv[0]);
        exit(0); // NOLINT
    }

    opt->backlog = BACKLOG;
    opt->bufsize = BUFSIZE;

    for (int i = 1; i < argc; i++) {
        const char * arg = argv[i];
        if (STR_CMP(-h, --help)) {
            show_usage(argv[0]);
            exit(0); // NOLINT
        }
        ARGV_TO_INT(-p, --port, port) // NOLINT
        ARGV_TO_INT(-l, --backlog, port) // NOLINT
        ARGV_TO_INT(-b, --bufsize, port) // NOLINT
        ARGV_TO_INT(-v, --verbos, verbos) // NOLINT
    }
}

void close_connection(connection_t * conn) {
    INFO(
        "%s relay success [%s<->%s] . read:%d write %d",
        conn->tag,
        conn->from_addr,
        conn->to_addr,
        conn->read_size,
        conn->write_size);
    if (conn->local > 0) {
        close(conn->local);
    }
    if (conn->target > 0) {
        close(conn->target);
    }
    free(conn);
}

void sockaddr_str(const struct sockaddr_in * addr, char * output, size_t output_size) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), buf, sizeof(buf));
    int port = ntohs(addr->sin_port);
    snprintf(output, output_size, "%s:%d", buf, port); // NOLINT
}

int connect_to_hostname(const char * hostname, const char * port) {
    struct addrinfo hints;
    struct addrinfo * res = nullptr;
    int sockfd = -1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // 支持 IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP 连接
    int status = getaddrinfo(hostname, port, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status)); // NOLINT
        return -1;
    }
    for (struct addrinfo * p = res; p != NULL; p = p->ai_next) {
        // 创建套接字
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("socket");
            WARN("connect error. try next...");
            continue; // 尝试下一个地址
        }

        // 尝试连接
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            close(sockfd); // 关闭失败的套接字
            sockfd = -1;
            continue; // 尝试下一个地址
        }
        // 连接成功
        break;
    }
    freeaddrinfo(res);

    INFO("connected to %s:%s succsss. %d", hostname, port, sockfd);
    return sockfd; // 返回已连接的套接字
}

void relay(connection_t * conn) {
    fd_set fds;
    int max_fd = (conn->local > conn->target) ? conn->local : conn->target;

    char buffer[conn->bufsize];

    while (1) {
        FD_ZERO(&fds);
        FD_SET(conn->local, &fds);
        FD_SET(conn->target, &fds);

        int activity = select(max_fd + 1, &fds, NULL, NULL, NULL); // NOLINT
        if (activity < 0) {
            break;
        }

        if (FD_ISSET(conn->local, &fds)) {
            ssize_t nlen = recv(conn->local, buffer, conn->bufsize, 0);
            conn->write_size += nlen;
            if (nlen <= 0) {
                break;
            }
            send(conn->target, buffer, nlen, 0);
        }

        if (FD_ISSET(conn->target, &fds)) {
            ssize_t nlen = recv(conn->target, buffer, conn->bufsize, 0);
            conn->read_size += nlen;
            if (nlen <= 0) {
                break;
            }
            send(conn->local, buffer, nlen, 0);
        }
    }

    close_connection(conn);
}
