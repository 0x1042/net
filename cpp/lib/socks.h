#pragma once

#include <asio.hpp>

#include "relay.h"

namespace socks {

auto handle(TcpStream socket) -> asio::awaitable<void>;

} // namespace socks
