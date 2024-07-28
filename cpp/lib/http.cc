#include "http.h"

#include <cstddef>
#include <string>
#include <vector>

#include <asio/buffer.hpp>
#include <asio/use_awaitable.hpp>
#include <spdlog/spdlog.h>

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
        std::vector<std::string> tokens = split(lines[0], ' ');
        this->method = tokens[0];
    }

    {
        // Host: www.google.com:443
        // Host: www.google.com
        std::vector<std::string> tokens = split(lines[1], ':');
        if (tokens.size() == 3) {
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
    try {
        asio::streambuf buffer;

        std::vector<std::string> lines;
        lines.reserve(HEADER_CNT);
        for (;;) {
            std::size_t len = co_await asio::async_read_until(socket, buffer, "\r\n", asio::use_awaitable);
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
        asio::ip::tcp::resolver::results_type endpoints
            = co_await resolver.async_resolve(req.domain, req.port, asio::use_awaitable);

        TcpStream remote(socket.get_executor());

        co_await asio::async_connect(remote, endpoints, asio::use_awaitable);
        asio::ip::tcp::no_delay option(true);
        remote.set_option(option);

        spdlog::get("http")->info(
            "connect to {}:{} success. ", endpoints->endpoint().address().to_string(), endpoints->endpoint().port());

        if (req.method == CONNECT) {
            co_await asio::async_write(socket, asio::buffer(CONNECT_RESP), asio::use_awaitable);
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

            co_await asio::async_write(remote, asio::buffer(headers), asio::use_awaitable);
        }

        co_await relay(socket, remote, "http");

    } catch (const std::exception & ex) {
        spdlog::get("http")->error("run exception :{}", ex.what());
    }
}

} // namespace http
