#include "http.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "log.h"
#include "relay.h"

CONST char CONNECT[] = "CONNECT";
CONST char CONNECT_RESP[] = "HTTP/1.1 200 Connection Established\r\n\r\n";

void parse_http_header(const char * header, http_header_t * request) {
    assert(header != NULL);
    assert(request != NULL);

    memset(request, 0, sizeof(http_header_t));

    const char * line = header;
    const char * next_line = strstr(line, "\r\n");

    if (next_line) {
        sscanf(line, "%15s %255s %15s", request->method, request->url, request->version);
        line = next_line + 2;
    }

    while (line && (next_line = strstr(line, "\r\n"))) {
        char key[256] = {0};
        char value[256] = {0};
        sscanf(line, "%255[^:]: %255[^\r\n]", key, value);

        if (strcasecmp(key, "Host") == 0) {
            char hostname[256] = {0};
            char port[16] = "80";
            if (sscanf(value, "%255[^:]:%s", hostname, port) == 2) {
                snprintf(request->host, sizeof(request->host), "%s", hostname);
            } else {
                snprintf(request->host, sizeof(request->host), "%s", value);
            }
            strcpy(request->port, port);

        } else if (strcasecmp(key, "User-Agent") == 0) {
            snprintf(request->user_agent, sizeof(request->user_agent), "%s", value);
        } else if (strcasecmp(key, "Connection") == 0) {
            snprintf(request->connection, sizeof(request->connection), "%s", value);
        }

        line = next_line + 2;
    }
}

void debug_http_header(const http_header_t * request) {
    if (request == NULL) {
        return;
    }

    TRACE("method:%s", request->method);
    TRACE("host:%s", request->host);
    TRACE("url:%s", request->url);
    TRACE("version:%s", request->version);
    TRACE("user_agent:%s", request->user_agent);
    TRACE("connection:%s", request->connection);
}

void * handle_http(void * arg) {
    auto conn = (connection_t *)arg;

    char header[LINE_BUF];
    ssize_t len = recv(conn->local, &header, sizeof(header), 0);

    if (len <= 0) {
        return NULL;
    }

    http_header_t request;
    parse_http_header(header, &request);
    debug_http_header(&request);

    strcpy(conn->to_addr, request.host);

    int remote = connect_to_hostname(request.host, request.port);
    conn->target = remote;

    if (strcmp(CONNECT, request.method) == 0) {
        send(conn->local, CONNECT_RESP, strlen(CONNECT_RESP), 0); // NOLINT
    } else {
        send(conn->target, header, strlen(header), 0);
    }

    relay(conn);

    return NULL;
}
