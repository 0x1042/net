//
// Created by 韦轩 on 2024/11/26.
//

#pragma once

#include <sys/types.h>

#include "relay.h"

int new_server(option_t * option);

void start(option_t * option, int fd);
