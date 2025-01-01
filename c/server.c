#include "server.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/types.h>

#include "http.h"
#include "log.h"
#include "relay.h"
#include "socks.h"

void close_server(const int fd, const char * msg) {
    INFO("close server. %s", msg);
    close(fd);
}

int new_server(option_t * option) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(option->port);

    option->fd = socket(AF_INET6, SOCK_STREAM, 0);

    if (option->fd < 0) {
        close_server(option->fd, "create socket failed");
        return -1;
    }

    if (bind(option->fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close_server(option->fd, "bind failed");
        return -1;
    }

    if (listen(option->fd, option->backlog) != 0) {
        close_server(option->fd, "listen failed");
        return -1;
    }

    INFO("server %d listening on port %d", getpid(), option->port);

    return option->fd;
}

int start(const option_t * option) {
    for (;;) {
        struct sockaddr_in addr;
        socklen_t client_len = sizeof(addr);

        const int incoming = accept(option->fd, (struct sockaddr *)&addr, &client_len);

        if (incoming < 0) {
            ERROR("accept failed");
            break;
        }

        char flag = 0;
        const ssize_t len = recv(incoming, &flag, 1, MSG_PEEK);
        if (len < 0) {
            ERROR("peek failed");
            continue;
        }

        connection_t * conn = malloc(sizeof(connection_t)); // NOLINT
        init_conn(conn, incoming);
        conn->bufsize = option->bufsize;
        sockaddr_str(&addr, conn->from_addr, sizeof(conn->from_addr));

        INFO("incoming request. fd:%d, from: %s type %d", incoming, conn->from_addr, flag);

#ifdef __APPLE__
        pthread_t worker = NULL; // NOLINT
#else
        pthread_t worker = 0;
#endif
        if (flag == SOCKS_VER) {
            strncpy(conn->tag, "socks", sizeof(conn->tag) - 1);
            conn->tag[5] = '\0';
            pthread_create(&worker, NULL, handle_socks, conn); // NOLINT
        } else {
            strncpy(conn->tag, "http", sizeof(conn->tag) - 1);
            conn->tag[4] = '\0';
            pthread_create(&worker, NULL, handle_http, conn); // NOLINT
        }
        pthread_detach(worker);
    }

    close(option->fd);

    return -1;
}
