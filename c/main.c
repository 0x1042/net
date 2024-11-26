#include "server.h"
#include "log.h"
#include <stdlib.h>

int main(int argc, char **argv) {
	option_t *option = malloc(sizeof(option_t));
	option->port = 10080;
	option->backlog = 1024;
	option->bufsize = 4096;

	if (argc > 1) {
		option->port = atoi(argv[1]); // NOLINT
	}
	int server = new_server(option);
	if (server < 0) {
		ERROR("create server failed. %d", server);
		exit(-1);
	}
	start(option, server);
	free(option);
	return 0;
}
