//
// Created by 韦轩 on 2024/11/26.
//

#pragma once

#include <sys/types.h>

typedef struct {
	int local;
	int target;
	ssize_t read_size;
	ssize_t write_size;
	int bufsize;
} connection_t;

typedef struct {
	int port;
	int backlog;
	int bufsize;
} option_t;

int new_server(option_t *option);

void start(option_t *option, int fd);

void *handle(void *arg);

void close_connection(connection_t *conn);
