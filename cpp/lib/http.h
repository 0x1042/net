#pragma once
#include <asio.hpp>

#include "relay.h"

namespace http {

struct Req {
    std::string method;
    std::string path;
    std::string domain;
    std::string port;
    std::vector<std::string> heads;

    explicit Req(const std::vector<std::string> & lines);
};

auto handle(TcpStream socket) -> asio::awaitable<void>;

} // namespace http