#include "relay.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <spdlog/spdlog.h>

#include "http.h"
#include "socks.h"

using namespace asio::experimental::awaitable_operators;

constexpr size_t bufsize = 16384;
constexpr uint8_t ver = 0x05;

auto relay(asio::ip::tcp::socket & from, asio::ip::tcp::socket & to, const std::string & tag) -> asio::awaitable<void> {
    auto relay = [tag](asio::ip::tcp::socket & from, asio::ip::tcp::socket & to) -> asio::awaitable<void> {
        const auto & from_addr = from.remote_endpoint();
        const auto & to_addr = to.remote_endpoint();
        size_t cnt = 0;
        try {
            std::array<std::byte, bufsize> data{};
            for (;;) {
                std::size_t n = co_await from.async_read_some(asio::buffer(data), asio::use_awaitable);
                co_await asio::async_write(to, asio::buffer(data, n), asio::use_awaitable);
                cnt += n;
            }
        } catch (...) {
            from.close();
            to.close();
        }

        spdlog::get(tag)->info(
            "{}:{} -> {}:{} transfer {} bytes success. ",
            from_addr.address().to_string(),
            from_addr.port(),
            to_addr.address().to_string(),
            to_addr.port(),
            cnt);
    };

    co_await (relay(from, to) && relay(to, from));
}

auto listener(asio::io_context & io_context, unsigned short port) -> asio::awaitable<void> {
    auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
    spdlog::info("server listen at {}:{}", endpoint.address().to_string(), endpoint.port());

    asio::ip::tcp::acceptor acceptor(io_context, endpoint);
    for (;;) {
        asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);

        std::array<std::byte, 1> data{};
        socket.receive(asio::buffer(data), asio::socket_base::message_peek);
        const auto & endpoint = socket.remote_endpoint();
        spdlog::info("incoming request. {}:{}", endpoint.address().to_string(), endpoint.port());

        if (std::to_integer<uint8_t>(data.at(0)) == ver) {
            asio::co_spawn(io_context, socks::handle(std::move(socket)), asio::detached);
        } else {
            asio::co_spawn(io_context, http::handle(std::move(socket)), asio::detached);
        }
    }
}

auto trim(const std::string & str) -> std::string {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start) == 1) {
        start++;
    }

    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && (std::isspace(*end) == 1));

    return {start, end + 1};
}

auto split(const std::string & line, char delimiter) -> std::vector<std::string> {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(line);

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}
