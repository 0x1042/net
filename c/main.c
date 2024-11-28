#include <getopt.h>
#include <stdlib.h>

#include "log.h"
#include "relay.h"
#include "server.h"

int main(int argc, char ** argv) {
    option_t opt;
    parse_command_line(argc, argv, &opt);
    int server = new_server(&opt);
    if (server < 0) {
        ERROR("create server failed. %d", server);
        exit(-1); // NOLINT
    }
    start(&opt, server);
    return 0;
}
