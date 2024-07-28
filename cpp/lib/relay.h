#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

#include <asio.hpp>

struct Option {
    uint16_t port = 10080; // NOLINT
    bool fastopen = false;
    bool nodelay = false;
    bool reuseaddr = false;
    uint32_t worker = 4; // NOLINT

    Option(int argc, char ** argv);

    static void showUsage(const std::string & name);
};

using TcpStream = asio::ip::tcp::socket;
using EndPoint = asio::ip::tcp::endpoint;

auto relay(TcpStream & from, TcpStream & to, const std::string & tag = "console") -> asio::awaitable<void>;

auto listener(asio::io_context & io_context, const Option & option) -> asio::awaitable<void>;

auto trim(const std::string & str) -> std::string;

auto split(const std::string & line, char delimiter) -> std::vector<std::string>;