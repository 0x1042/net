#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <asio.hpp>

#include "lib/relay.h"

void show_usage(const std::string & name) {
    std::cout << "Usage: " << name << " <option> [value]" << std::endl;
    std::cout << "Options:\n";
    std::cout << "  -p, --port          listen port\n";
    std::cout << "  -f, --fastopen      enable fastopen\n";
    std::cout << "  -r, --reuseaddr     enable reuse address\n";
    std::cout << "  -n, --nodelay       enable tcp nodelay\n";
    std::cout << "  -w, --worker        worker number\n";
    std::cout << "  -h, --help          print help\n";
}

constexpr uint16_t DEFAULT_PORT = 10080;
constexpr size_t WORKER_NUM = 4;

auto main(int argc, char ** argv) -> int {
    size_t worker = WORKER_NUM;

    if (argc < 2) {
        show_usage(argv[0]);
        return 0;
    }

    Option option{.port = DEFAULT_PORT, .fastopen = false, .nodelay = false, .reuseaddr = false};

    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_usage(argv[0]);
            return 0;
        }
        if ((arg == "-w" || arg == "--worker") && i + 1 < argc) {
            worker = std::stoi(argv[i + 1]);
            continue;
        }
        if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            option.port = std::stoi(argv[i + 1]);
            continue;
        }
        if ((arg == "-f" || arg == "--fastopen")) {
            option.fastopen = true;
            continue;
        }
        if ((arg == "-r" || arg == "--reuseaddr")) {
            option.reuseaddr = true;
            continue;
        }
        if ((arg == "-n" || arg == "--nodelay")) {
            option.nodelay = true;
            continue;
        }
    }

    auto http = spdlog::stdout_color_mt("http");
    auto socks = spdlog::stdout_color_mt("socks");
    auto console = spdlog::stdout_color_mt("main");
    spdlog::set_default_logger(console);
    spdlog::set_pattern("%^[%H:%M:%S %e] %l [%n] thread-%t %v %$");

    try {
        asio::io_context io_context;

        // Run listener coroutine
        asio::co_spawn(io_context, listener(io_context, option), asio::detached);

        std::vector<std::thread> threads;
        threads.resize(worker);
        for (size_t i = 0; i < worker; ++i) {
            threads.emplace_back([&io_context]() { io_context.run(); });
        }

        io_context.run();

        // Join all threads
        for (auto & worker : threads) {
            worker.join();
        }
    } catch (const std::exception & ex) {
        spdlog::error("run exception :{}", ex.what());
    }
    return 0;
}