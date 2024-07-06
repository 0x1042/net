cmake_minimum_required(VERSION 3.26)
project(server LANGUAGES C CXX ASM)

add_compile_options(-Wall -Wextra -pedantic -g)
add_compile_options(-fsized-deallocation)
add_compile_options(-Werror=delete-non-virtual-dtor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

file(
	GLOB
	SOURCES
	"lib/*.cc"
)

include_directories(".")

add_executable(server main.cc ${SOURCES})

target_link_libraries(
		${PROJECT_NAME}
		PRIVATE Threads::Threads
		PRIVATE asio::asio
		PRIVATE spdlog::spdlog
)