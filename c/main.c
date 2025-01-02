#include "log.h"
#include "relay.h"
#include "server.h"

int main(const int argc, char ** argv) {
    option_t opt;
    parse_command_line(argc, argv, &opt);

    if (opt.verbos == 1) {
        set_level(LEVEL_TRACE);
    }

    const int server = new_server(&opt);
    if (server < 0) {
        ERROR("create server failed. %d", server);
        return -1;
    }
    return start(&opt);
}
