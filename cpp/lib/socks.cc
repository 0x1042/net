#include "socks.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <asio/ip/address_v4.hpp>
#include <spdlog/spdlog.h>

#include "relay.h"

namespace socks {

constexpr uint8_t FIVE = 0x05;
constexpr uint8_t ZERO = 0x00;
constexpr uint8_t ONE = 0x01;
constexpr uint8_t TEN = 10;

constexpr size_t IPV4_LEN = 4;
constexpr size_t IPV6_LEN = 16;
constexpr size_t PORT_LEN = 2;

auto handle(TcpStream socket) -> asio::awaitable<void> {
    try {
        // Perform SOCKS5 handshake
        std::array<uint8_t, 4> handshake_request{};
        co_await asio::async_read(socket, asio::buffer(handshake_request), asio::use_awaitable);

        if (handshake_request[0] != FIVE) {
            co_return; // Not SOCKS5
        }
        std::array<uint8_t, 2> handshake_response = {FIVE, ZERO};
        co_await asio::async_write(socket, asio::buffer(handshake_response), asio::use_awaitable);

        // Read SOCKS5 request
        std::array<uint8_t, 4> request{};
        co_await asio::async_read(socket, asio::buffer(request), asio::use_awaitable);

        if (request[1] != ONE) {
            co_return; // Only support CONNECT command
        }

        uint8_t adty = request[3];
        EndPoint endpoint;
        if (adty == 0x01) {
            std::array<uint8_t, IPV4_LEN> address{};
            co_await asio::async_read(socket, asio::buffer(address), asio::use_awaitable);

            std::array<uint8_t, PORT_LEN> port{};
            co_await asio::async_read(socket, asio::buffer(port), asio::use_awaitable);

            endpoint.port((port[0] << sizeof(size_t)) | port[1]);
            endpoint.address(asio::ip::make_address_v4(address));
        }

        if (adty == 0x03) {
            std::array<uint8_t, IPV6_LEN> address{};
            co_await asio::async_read(socket, asio::buffer(address), asio::use_awaitable);

            std::array<uint8_t, PORT_LEN> port{};
            co_await asio::async_read(socket, asio::buffer(port), asio::use_awaitable);

            endpoint.port((port[0] << sizeof(size_t)) | port[1]);
            endpoint.address(asio::ip::make_address_v6(address));
        }

        if (adty == 0x04) {
            std::array<uint8_t, 1> len{};
            co_await asio::async_read(socket, asio::buffer(len), asio::use_awaitable);

            std::vector<uint8_t> address;
            address.resize(len[0]);
            co_await asio::async_read(socket, asio::buffer(address), asio::use_awaitable);

            std::array<uint8_t, PORT_LEN> port{};
            co_await asio::async_read(socket, asio::buffer(port), asio::use_awaitable);

            asio::ip::tcp::resolver resolver(socket.get_executor());

            std::string domain(address.begin(), address.end());
            std::string port_str(std::to_string(port[0] << sizeof(size_t) | port[1]));

            asio::ip::tcp::resolver::results_type endpoints
                = co_await resolver.async_resolve(domain, port_str, asio::use_awaitable);
            endpoint = endpoints->endpoint();
        }

        // Read address and port
        // spdlog::get("socks")->info("connect to {}:{} success. ", endpoint.address().to_string(), endpoint.port());

        TcpStream remote(socket.get_executor());

        // Connect to the remote server
        co_await remote.async_connect(endpoint, asio::use_awaitable);
        asio::ip::tcp::no_delay option(true);
        remote.set_option(option);

        // Send success response to the client
        std::array<uint8_t, TEN> response = {FIVE, ZERO, ZERO, ONE, ZERO, ZERO, ZERO, ZERO, ZERO, ZERO};
        co_await asio::async_write(socket, asio::buffer(response), asio::use_awaitable);

        // Relay traffic between client and remote server
        co_await relay(socket, remote, "socks");
    } catch (const std::exception & ex) {
        spdlog::get("socks")->error("run exception :{}", ex.what());
    }
}
} // namespace socks