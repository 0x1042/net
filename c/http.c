#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "log.h"
#include "relay.h"

CONST char CONNECT[] = "CONNECT";
CONST char CONNECT_RESP[] = "HTTP/1.1 200 Connection Established\r\n\r\n";

void parse_http_header(const char * header, http_header_t * request) {
    memset(request, 0, sizeof(http_header_t));

    const char * line_start = header;
    const char * line_end = nullptr;

    while ((line_end = strchr(line_start, '\n')) != nullptr) {
        size_t line_length = line_end - line_start;

        if (line_length > 0 && line_start[line_length - 1] == '\r') {
            line_length--;
        }

        char line[LINE_BUF] = {0};
        strncpy(line, line_start, line_length);

        if (strncmp(line, "CONNECT", 7) == 0) {
            sscanf(line, "%15s %255s %15s", request->method, request->url, request->version);
        } else if (strncmp(line, "GET", 3) == 0) {
            sscanf(line, "%15s %255s %15s", request->method, request->url, request->version);
        } else if (strncmp(line, "POST", 4) == 0) {
            sscanf(line, "%15s %255s %15s", request->method, request->url, request->version);
        } else if (strncmp(line, "PUT", 3) == 0) {
            sscanf(line, "%15s %255s %15s", request->method, request->url, request->version);
        } else if (strncmp(line, "Host:", 5) == 0) {
            sscanf(line, "Host: %255s", request->host);
        } else if (strncmp(line, "User-Agent:", 11) == 0) {
            sscanf(line, "User-Agent: %255[^\r\n]", request->user_agent);
        } else if (strncmp(line, "Proxy-Connection:", 17) == 0) {
            sscanf(line, "Proxy-Connection: %63s", request->connection);
        }

        line_start = line_end + 1;
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

void parse_host_port(const char * input, host_port_t * result) {
    // 初始化结果
    memset(result, 0, sizeof(host_port_t));

    // 找到 ':' 的位置
    const char * colon = strchr(input, ':');
    if (colon != NULL) {
        // 有端口号
        size_t host_len = colon - input; // 计算主机名长度
        strncpy(result->host, input, host_len); // 提取主机名
        result->host[host_len] = '\0'; // 确保字符串以 '\0' 结束
        // 提取端口号
        result->port = atoi(colon + 1); // 将端口号从字符串转换为整数 // NOLINT
    } else {
        // 没有端口号，默认使用 80
        strncpy(result->host, input, sizeof(result->host) - 1);
        result->host[sizeof(result->host) - 1] = '\0'; // 防止溢出
        result->port = HTTP_PORT; // 默认端口
    }
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

    host_port_t hp;

    strcpy(conn->to_addr, request.host);

    parse_host_port(request.host, &hp);

    char port[6]; // NOLINT
    snprintf(port, sizeof(port), "%d", hp.port); // NOLINT

    int remote = connect_to_hostname(hp.host, port);
    conn->target = remote;

    if (strcmp(CONNECT, request.method) == 0) {
        send(conn->local, CONNECT_RESP, strlen(CONNECT_RESP), 0); // NOLINT
    } else {
        send(conn->target, header, strlen(header), 0);
    }

    relay(conn);

    return NULL;
}
