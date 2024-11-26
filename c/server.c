//
// Created by 韦轩 on 2024/11/26.
//

#include "server.h"

#include "log.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const static char NO_AUTH[2] = {0x05, 0x00};
const static char FAILURE_CMD[2] = {0x05, 0x07};
const static char FAILURE_ATYP[2] = {0x05, 0x08};
const static char SUCCESS[10] = {0x05, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0};

void sockaddr_str(const struct sockaddr_in *addr, char *output,
				  size_t output_size);

void sockaddr_str(const struct sockaddr_in *addr, char *output,
				  size_t output_size) {
	char buf[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(addr->sin_addr), buf, sizeof(buf));
	int port = ntohs(addr->sin_port);
	snprintf(output, output_size, "%s:%d", buf, port);
}

void close_connection(connection_t *conn) {
	INFO("client-%d is closed. read:%d write %d", conn->local,
		 conn->read_size, conn->write_size);
	if (conn->local > 0) {
		close(conn->local);
	}
	if (conn->target > 0) {
		close(conn->target);
	}
	free(conn);
}

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

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
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
		int incoming = accept(fd, (struct sockaddr *) &addr, &client_len);
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
		pthread_create(&worker, NULL, handle, conn); // NOLINT
		pthread_detach(worker);
	}
}

void *handle(void *arg) {
	connection_t *conn = (connection_t *) arg;
	char shake[2];

	// Step 1: SOCKS5 handshake
	ssize_t len = recv(conn->local, shake, 2, 0); // 客户端握手
	if (len != 2) {
		close_connection(conn);
		ERROR("handshake failed");
		return NULL;
	}

	TRACE("client-%d handshake success.", conn->local);

	int nmethod = (int) shake[1];

	char auth[nmethod];
	len = recv(conn->local, auth, nmethod, 0);

	// todo auth
	if (len != nmethod) {
		close_connection(conn);
		ERROR("handshake auth failed");
		return NULL;
	}

	send(conn->local, NO_AUTH, sizeof(NO_AUTH), 0);
	TRACE("client-%d auth success.", conn->local);

	char header[4];
	// Step 2: SOCKS5 request
	len = recv(conn->local, header, 4, 0); // 客户端请求
	if (len != 4) {
		close_connection(conn);
		ERROR("client-%d read request failed ", conn->local);
		return NULL;
	}

	if (header[1] != 0x01) { // 仅支持 CONNECT
		send(conn->local, FAILURE_CMD, sizeof(FAILURE_CMD), 0);
		close_connection(conn);
		return NULL;
	}

	if (header[3] != 0x01) {
		send(conn->local, FAILURE_ATYP, sizeof(FAILURE_ATYP), 0);
		close_connection(conn);
		return NULL;
	}

	char addr_info[6];
	len = recv(conn->local, addr_info, 6, 0); // ipv4

	if (len != 6) {
		close_connection(conn);
		return NULL;
	}

	struct sockaddr_in remote = {0};
	remote.sin_family = AF_INET;

	memcpy(&remote.sin_addr, &addr_info, 4);
	remote.sin_port = *(uint16_t *) (addr_info + 4);

	char result[64];
	sockaddr_str(&remote, result, sizeof(result));

	INFO("client-%d start connect to %s ", conn->local, result);

	// 连接目标服务器
	conn->target = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(conn->target, (struct sockaddr *) &remote, sizeof(remote)) <
		0) {
		ERROR("Connect failed");
		close_connection(conn);
		return NULL;
	}

	INFO("client-[%d] connect to %s success", conn->local, result);

	send(conn->local, SUCCESS, sizeof(SUCCESS), 0);

	// Step 3: 数据转发（全双工）
	fd_set fds;
	int max_fd =
		(conn->local > conn->target) ? conn->local : conn->target;

	char buffer[conn->bufsize];

	while (1) {
		FD_ZERO(&fds);
		FD_SET(conn->local, &fds);
		FD_SET(conn->target, &fds);

		int activity = select(max_fd + 1, &fds, NULL, NULL, NULL); // NOLINT
		if (activity < 0)
			break;

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
	return NULL;
}