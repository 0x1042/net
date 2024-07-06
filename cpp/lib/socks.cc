#include "socks.h"

#include <spdlog/spdlog.h>

#include "relay.h"

namespace socks {

constexpr uint8_t FIVE = 0x05;
constexpr uint8_t ZERO = 0x00;
constexpr uint8_t ONE = 0x01;
constexpr uint8_t TEN = 10;

auto handle(asio::ip::tcp::socket socket) -> asio::awaitable<void> {
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

        // Read address and port
        std::array<uint8_t, 4> address{};
        co_await asio::async_read(socket, asio::buffer(address), asio::use_awaitable);

        std::array<uint8_t, 2> port{};
        co_await asio::async_read(socket, asio::buffer(port), asio::use_awaitable);

        asio::ip::tcp::endpoint remote_endpoint(asio::ip::make_address_v4(address), (port[0] << 8) | port[1]);

        spdlog::get("socks")->info(
            "connect to {}:{} success. ", remote_endpoint.address().to_string(), remote_endpoint.port());

        asio::ip::tcp::socket remote(socket.get_executor());

        // Connect to the remote server
        co_await remote.async_connect(remote_endpoint, asio::use_awaitable);
        asio::ip::tcp::no_delay option(true);
        remote.set_option(option);

        // Send success response to the client
        std::array<uint8_t, TEN> response
            = {FIVE, ZERO, ZERO, ONE, address[0], address[1], address[2], address[3], port[0], port[1]};
        co_await asio::async_write(socket, asio::buffer(response), asio::use_awaitable);

        // Relay traffic between client and remote server
        co_await relay(socket, remote, "socks");
    } catch (const std::exception & ex) {
        spdlog::get("socks")->error("run exception :{}", ex.what());
    }
}
} // namespace socks