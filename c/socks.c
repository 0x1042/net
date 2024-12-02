#include "socks.h"
#include "log.h"

#include "relay.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>

const static char NO_AUTH[2] = {0x05, 0x00};
const static char FAILURE_CMD[2] = {0x05, 0x07};
const static char FAILURE_ATYP[2] = {0x05, 0x08};
const static char SUCCESS[10] = {0x05, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0};

void *handle_socks(void *arg) {
  connection_t *conn = (connection_t *)arg;
  char shake[2];

  // Step 1: SOCKS5 handshake
  ssize_t len = recv(conn->local, shake, 2, 0); // 客户端握手
  if (len != 2) {
    close_connection(conn);
    ERROR("handshake failed");
    return NULL;
  }

  TRACE("client-%d handshake success.", conn->local);

  int nmethod = (int)shake[1];

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
  remote.sin_port = *(uint16_t *)(addr_info + 4);

  char result[64];
  sockaddr_str(&remote, result, sizeof(result));

  INFO("client-%d start connect to %s ", conn->local, result);

  // 连接目标服务器
  conn->target = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(conn->target, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
    ERROR("Connect failed");
    close_connection(conn);
    return NULL;
  }

  INFO("client-[%d] connect to %s success", conn->local, result);

  send(conn->local, SUCCESS, sizeof(SUCCESS), 0);

  relay(conn);
  return NULL;
}