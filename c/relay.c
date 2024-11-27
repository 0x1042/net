#include "relay.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void close_connection(connection_t *conn) {
  INFO("client-%d is closed. read:%d write %d", conn->local, conn->read_size,
       conn->write_size);
  if (conn->local > 0) {
    close(conn->local);
  }
  if (conn->target > 0) {
    close(conn->target);
  }
  free(conn);
}

void sockaddr_str(const struct sockaddr_in *addr, char *output,
                  size_t output_size) {
  char buf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(addr->sin_addr), buf, sizeof(buf));
  int port = ntohs(addr->sin_port);
  snprintf(output, output_size, "%s:%d", buf, port);
}
