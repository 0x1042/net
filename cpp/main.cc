#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <asio.hpp>

#include "lib/relay.h"

auto main(int argc, char ** argv) -> int {
    Option option = Option(argc, argv);
    auto http = spdlog::stdout_color_mt("http");
    auto socks = spdlog::stdout_color_mt("socks");
    auto console = spdlog::stdout_color_mt("main");
    spdlog::set_default_logger(console);
    spdlog::set_pattern("%^[%H:%M:%S %e] %l [%n] thread-%t %v %$");

    asio::io_context io_context;

    // Run listener coroutine
    asio::co_spawn(io_context, listener(io_context, option), asio::detached);

    std::vector<std::thread> threads;
    uint32_t worker = option.worker == 0 ? std::thread::hardware_concurrency() : option.worker;

    threads.resize(worker);
    for (size_t i = 0; i < worker; ++i) {
        threads.emplace_back([&io_context]() { io_context.run(); });
    }

    io_context.run();

    // Join all threads
    for (auto & worker : threads) {
        worker.join();
    }
    return 0;
}
