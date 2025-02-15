#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "relay.h"
#include "server.h"

#define STR_CMP(s, l) (strcmp(arg, #s) == 0 || strcmp(arg, #l) == 0)

#define ARGV_TO_INT(s, l, d) \
    if (STR_CMP(s, l) && i + 1 < argc) { \
        opt->d = atoi(argv[i + 1]); \
    }

#define GREEN(message) printf("\033[32m%s\033[0m\n", message)

void show_usage(const char * name) {
    printf("\033[36mUsage: %s <option> [value]\033[0m\n", name);
    printf("\n");
    GREEN("Options:");
    GREEN("  -p, --port          listen port");
    GREEN("  -b, --bufsize       buffer size");
    GREEN("  -l, --backlog       backlog size");
    GREEN("  -v, --verbos        verbos log");
    GREEN("  -f, --fastopen      enable fastopen");
    GREEN("  -r, --reuseaddr     enable reuse address");
    GREEN("  -n, --nodelay       enable tcp nodelay");
    GREEN("  -w, --worker        worker number");
    GREEN("  -h, --help          print help");
}

int parse_command_line(int argc, char ** argv, option_t * opt) {
    if (argc < 2) {
        show_usage(argv[0]);
        return 1;
    }

    opt->backlog = BACKLOG;
    opt->bufsize = BUFSIZE;

    for (int i = 1; i < argc; i++) {
        const char * arg = argv[i];
        if (STR_CMP(-h, --help)) {
            show_usage(argv[0]);
            return 1;
        }
        ARGV_TO_INT(-p, --port, port) // NOLINT
        ARGV_TO_INT(-l, --backlog, port) // NOLINT
        ARGV_TO_INT(-b, --bufsize, port) // NOLINT
        ARGV_TO_INT(-v, --verbos, verbos) // NOLINT
    }
    return 0;
}

int main(const int argc, char ** argv) {
    option_t opt;
    int status = parse_command_line(argc, argv, &opt);

    if (status != 0) {
        exit(0);
    }

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
