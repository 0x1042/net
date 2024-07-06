#pragma once

#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>

#include <asio.hpp>

namespace socks {

auto handle(asio::ip::tcp::socket socket) -> asio::awaitable<void>;

}
