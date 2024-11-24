#include <string>
#include <thread>
#include <vector>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <asio.hpp>

#include "lib/relay.h"

auto main(int argc, char ** argv) -> int {
    Option option = Option(argc, argv);
    auto console = spdlog::stdout_color_mt("main");
    spdlog::set_default_logger(console);
    spdlog::set_pattern("%^[%H:%M:%S %e] thd-%t %v %$");

    asio::io_context io_context;

    // Run listener coroutine
    asio::co_spawn(io_context, listener(io_context, option), asio::detached);

    std::vector<std::jthread> workers;
    uint32_t worker = option.worker == 0 ? std::thread::hardware_concurrency() : option.worker;

    workers.resize(worker);
    for (size_t i = 0; i < worker; ++i) {
        workers.emplace_back([&io_context]() { io_context.run(); });
    }

    io_context.run();
    return 0;
}
