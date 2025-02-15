#pragma once

typedef struct {
    char method[16];
    char url[256];
    char version[16];
    char host[256];
    char port[16];
    char user_agent[256];
    char connection[64];
} http_header_t;

typedef struct {
    char host[256]; // 主机名
    int port; // 端口号
} host_port_t;

void parse_http_header(const char * header, http_header_t * request);

void debug_http_header(const http_header_t * request);

void * handle_http(void * arg);
