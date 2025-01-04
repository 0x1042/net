#include "socks.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "log.h"
#include "relay.h"

CONST char CMD_CONNECT = 0x01;
CONST char NO_AUTH[2] = {0x05, 0x00};
CONST char FAILURE_CMD[2] = {0x05, 0x07};
CONST char FAILURE_ATYP[2] = {0x05, 0x08};
CONST char SUCCESS[10] = {0x05, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0};

void * handle_socks(void * arg) {
    auto conn = (connection_t *)arg;
    char shake[2];

    // Step 1: SOCKS5 handshake
    if (recv(conn->local, shake, 2, 0) != 2) {
        close_connection(conn);
        ERROR("handshake failed");
        return NULL;
    }

    TRACE("client-%d handshake success.", conn->local);

    const int nmethod = (int)shake[1];

    char auth[nmethod];
    if (recv(conn->local, auth, nmethod, 0) != nmethod) {
        close_connection(conn);
        ERROR("handshake failed");
        return NULL;
    }

    send(conn->local, NO_AUTH, sizeof(NO_AUTH), 0);
    TRACE("client-%d auth success.", conn->local);

    char header[4];

    // Step 2: SOCKS5 request
    if (recv(conn->local, header, 4, 0) != 4) {
        close_connection(conn);
        ERROR("client-%d read request failed ", conn->local);
        return NULL;
    }

    if (header[1] != CMD_CONNECT) { // 仅支持 CONNECT
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
    if (recv(conn->local, addr_info, 6, 0) != 6) {
        close_connection(conn);
        return NULL;
    }

    struct sockaddr_in remote = {0};
    remote.sin_family = AF_INET;

    memcpy(&remote.sin_addr, &addr_info, 4);
    remote.sin_port = *(uint16_t *)(addr_info + 4);

    sockaddr_str(&remote, conn->to_addr, sizeof(conn->to_addr));

    INFO("client-%d start connect to %s ", conn->local, conn->to_addr);

    // 连接目标服务器
    conn->target = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(conn->target, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
        ERROR("Connect failed");
        close_connection(conn);
        return NULL;
    }

    INFO("client-[%d] connect to %s success", conn->local, conn->to_addr);

    send(conn->local, SUCCESS, sizeof(SUCCESS), 0);

    relay(conn);
    return NULL;
}
