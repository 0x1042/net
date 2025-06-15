#include "relay.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#include <asio/as_tuple.hpp>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "http.h"
#include "logger.h"
#include "socks.h"

using namespace asio::experimental::awaitable_operators;

constexpr size_t bufsize = 16384;
constexpr uint8_t ver = 0x05;

Option::Option(int argc, char ** argv) {
    if (argc < 2) {
        Option::showUsage(argv[0]);
        exit(0); // NOLINT
    }
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            showUsage(argv[0]);
            exit(0); // NOLINT
        }
        if ((arg == "-w" || arg == "--worker") && i + 1 < argc) {
            worker = std::stoi(argv[i + 1]);
            continue;
        }
        if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port = std::stoi(argv[i + 1]);
            continue;
        }
        if ((arg == "-f" || arg == "--fastopen")) {
            fastopen = true;
            continue;
        }
        if ((arg == "-r" || arg == "--reuseaddr")) {
            reuseaddr = true;
            continue;
        }
        if ((arg == "-n" || arg == "--nodelay")) {
            nodelay = true;
            continue;
        }
    }
}

void Option::showUsage(const std::string & name) {
    std::cout << "Usage: " << name << " <option> [value]\n";
    std::cout << "Options:\n";
    std::cout << "  -p, --port          listen port\n";
    std::cout << "  -f, --fastopen      enable fastopen\n";
    std::cout << "  -r, --reuseaddr     enable reuse address\n";
    std::cout << "  -n, --nodelay       enable tcp nodelay\n";
    std::cout << "  -w, --worker        worker number\n";
    std::cout << "  -h, --help          print help\n";
}

auto relay(TcpStream & from, TcpStream & to, const std::string & tag) -> asio::awaitable<void> {
    std::string logger = tag;
    auto relay
        = [logger = std::move(logger)](TcpStream & from, TcpStream & to) -> asio::awaitable<void> {
        const auto & from_addr = from.remote_endpoint();
        const auto & to_addr = to.remote_endpoint();
        size_t cnt = 0;
        try {
            std::array<std::byte, bufsize> data{};
            for (;;) {
                auto [ec, rlen] = co_await from.async_read_some(
                    asio::buffer(data), asio::as_tuple(asio::use_awaitable));
                if (ec) {
                    break;
                }

                co_await asio::async_write(
                    to, asio::buffer(data, rlen), asio::as_tuple(asio::use_awaitable));
                cnt += rlen;
            }
        } catch (...) {
            from.close();
            to.close();
        }

        INFO(
            "{}:{} -> {}:{} transfer {} bytes success. ",
            from_addr.address().to_string(),
            from_addr.port(),
            to_addr.address().to_string(),
            to_addr.port(),
            cnt);
    };

    co_await (relay(from, to) && relay(to, from));
}

auto listener(asio::io_context & io_context, const Option & option) -> asio::awaitable<void> {
    auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), option.port);
    INFO("server listen at {}:{}", endpoint.address().to_string(), endpoint.port());

    asio::ip::tcp::acceptor ln(io_context, endpoint);

    if (option.reuseaddr) {
        asio::ip::tcp::acceptor::reuse_address reuse_opt(true);
        ln.set_option(reuse_opt);
    }

    for (;;) {
        auto && [ec, incoming] = co_await ln.async_accept(asio::as_tuple(asio::use_awaitable));

        if (ec) {
            break;
        }

        int enable = 1;
        if (option.fastopen) {
            ::setsockopt(
                incoming.native_handle(), IPPROTO_TCP, TCP_FASTOPEN, &enable, sizeof(enable));
        }

        if (option.nodelay) {
            ::setsockopt(
                incoming.native_handle(), IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
        }

        if (option.reuseaddr) {
            ::setsockopt(
                incoming.native_handle(), SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
            ::setsockopt(
                incoming.native_handle(), SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
        }

        std::array<std::byte, 1> data{};
        incoming.receive(asio::buffer(data), asio::socket_base::message_peek);

        if (std::to_integer<uint8_t>(data.at(0)) == ver) {
            asio::co_spawn(io_context, socks::handle(std::move(incoming)), asio::detached);
        } else {
            asio::co_spawn(io_context, http::handle(std::move(incoming)), asio::detached);
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
