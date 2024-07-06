#pragma once

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

auto relay(asio::ip::tcp::socket & from, asio::ip::tcp::socket & to, const std::string & tag = "console")
    -> asio::awaitable<void>;

auto listener(asio::io_context & io_context, unsigned short port) -> asio::awaitable<void>;

auto trim(const std::string & str) -> std::string;

auto split(const std::string & line, char delimiter) -> std::vector<std::string>;