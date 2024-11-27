#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

typedef struct {
  int local;
  int target;
  ssize_t read_size;
  ssize_t write_size;
  int bufsize;
} connection_t;

void close_connection(connection_t *conn);

void sockaddr_str(const struct sockaddr_in *addr, char *output,
                  size_t output_size);