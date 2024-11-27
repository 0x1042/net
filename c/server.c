//
// Created by 韦轩 on 2024/11/26.
//

#include "server.h"

#include "log.h"
#include "relay.h"
#include "socks.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void close_server(int fd, const char *msg) {
  INFO("close server. %s", msg);
  close(fd);
}

int new_server(option_t *option) {
  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(option->port);

  int fd = socket(AF_INET6, SOCK_STREAM, 0);

  if (fd < 0) {
    close_server(fd, "create socket failed");
    return -1;
  }

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    close_server(fd, "bind failed");
    return -1;
  }

  if (listen(fd, option->backlog) != 0) {
    close_server(fd, "listen failed");
    return -1;
  }

  INFO("server %d listening on port %d", getpid(), option->port);

  return fd;
}

void start(option_t *option, int fd) {
  struct sockaddr_in addr;
  socklen_t client_len = sizeof(addr);

  while (1) {
    int incoming = accept(fd, (struct sockaddr *)&addr, &client_len);
    char result[64];
    sockaddr_str(&addr, result, sizeof(result));
    INFO("incoming request. fd:%d, from: %s", incoming, result);
    if (incoming < 0) {
      ERROR("accept failed");
      break;
    }

    connection_t *conn = malloc(sizeof(connection_t));
    conn->local = incoming;
    conn->target = -1;
    conn->write_size = 0;
    conn->write_size = 0;
    conn->bufsize = option->bufsize;

    pthread_t worker;
    pthread_create(&worker, NULL, handle_socks, conn); // NOLINT
    pthread_detach(worker);
  }
}
