#include "http.h"

#include <string>
#include <vector>

#include <asio/buffer.hpp>
#include <asio/use_awaitable.hpp>

#include "logger.h"
#include "relay.h"

namespace http {

constexpr size_t HEADER_SIZE = 512;
constexpr size_t HEADER_CNT = 5;

static constexpr std::string_view CONNECT = "CONNECT";
static constexpr std::string_view CONNECT_RESP = "HTTP/1.1 200 Connection Established\r\n\r\n";

Req::Req(const std::vector<std::string> & lines) {
    // first line

    {
        // CONNECT www.google.com:443 HTTP / 1.1
        // GET http://www.google.com/ HTTP/1.1
        const std::vector<std::string> tokens = split(lines[0], ' ');
        this->method = tokens[0];
    }

    {
        // Host: www.google.com:443
        // Host: www.google.com
        if (const std::vector<std::string> tokens = split(lines[1], ':'); tokens.size() == 3) {
            this->domain = tokens[1];
            this->port = tokens[2];
        } else {
            this->domain = tokens[1];
            this->port = "80";
        }
    }

    // User-Agent: curl/8.6.0
    // Proxy-Connection: Keep-Alive
}

// todo http GET http://www.xxx.com/ HTTP/1.1
// https CONNECT www.xxx.com:443 HTTP/1.1
auto handle(TcpStream socket) -> asio::awaitable<void> {
    asio::streambuf buffer;

    std::vector<std::string> lines;
    lines.reserve(HEADER_CNT);
    for (;;) {
        auto [ec, len] = co_await asio::async_read_until(
            socket, buffer, "\r\n", asio::as_tuple(asio::use_awaitable));
        auto bufs = buffer.data();
        std::string line(asio::buffers_begin(bufs), asio::buffers_begin(bufs) + len); // NOLINT
        if (line == "\r\n" || line.empty()) {
            break;
        }
        lines.push_back(std::move(line));
        buffer.consume(len);
    }

    Req req(lines);
    asio::ip::tcp::resolver resolver(socket.get_executor());
    auto [ec, endpoints] = co_await resolver.async_resolve(
        req.domain, req.port, asio::as_tuple(asio::use_awaitable));
    if (ec) {
        ERROR("can not resolve {}:{}. {}{}", req.domain, req.port, ec.value(), ec.message());
        co_return;
    }

    TcpStream remote(socket.get_executor());

    co_await asio::async_connect(remote, endpoints, asio::as_tuple(asio::use_awaitable));
    asio::ip::tcp::no_delay option(true);
    remote.set_option(option);

    // spdlog::info(
    //     "connect to {}:{} success. ",
    //     endpoints->endpoint().address().to_string(),
    //     endpoints->endpoint().port());

    if (req.method == CONNECT) {
        co_await asio::async_write(
            socket, asio::buffer(CONNECT_RESP), asio::as_tuple(asio::use_awaitable));
    } else {
        std::string headers;
        headers.reserve(HEADER_SIZE);
        for (const auto & line : lines) {
            if (line == "Proxy-Connection: Keep-Alive\r\n") {
                continue;
            }
            headers += line;
        }
        headers += "\r\n";

        co_await asio::async_write(
            remote, asio::buffer(headers), asio::as_tuple(asio::use_awaitable));
    }

    co_await relay(socket, remote, "http");
}

} // namespace http
