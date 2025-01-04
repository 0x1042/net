#pragma once

#include <netinet/in.h>
#include <sys/types.h>

#if __clang_major__ >= 19
#    define CONST constexpr static
#else
#    define CONST const static
#endif

typedef struct {
    int local;
    int target;
    ssize_t read_size;
    ssize_t write_size;
    int bufsize;
    char tag[6];
    char from_addr[256];
    char to_addr[256];
} connection_t;

void init_conn(connection_t * conn, const int local);

typedef struct {
    int fd;
    int port;
    int backlog;
    int bufsize;
    int verbos;
} option_t;

typedef enum {
    SOCKS_VER = 0x05,
    BACKLOG = 1024,
    BUFSIZE = 1024 * 32,
    LINE_BUF = 1024,
    HTTP_PORT = 80,
} constval_t;

void show_usage(const char * name);

void parse_command_line(int argc, char ** argv, option_t * opt);

void close_connection(connection_t * conn);

void sockaddr_str(const struct sockaddr_in * addr, char * output, size_t output_size);

int connect_to_hostname(const char * hostname, const char * port);

void relay(connection_t * conn);
