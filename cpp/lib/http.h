#pragma once

#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>

#include <asio.hpp>

namespace http {

struct Req {
    std::string method;
    std::string path;
    std::string domain;
    std::string port;
    std::vector<std::string> heads;

    explicit Req(const std::vector<std::string> & lines);
};

auto handle(asio::ip::tcp::socket socket) -> asio::awaitable<void>;

} // namespace http