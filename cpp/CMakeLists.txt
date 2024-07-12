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

option(ENABLE_SANITIZE "enable address sanitize" OFF)
if(ENABLE_SANITIZE) 
  add_compile_options("-fno-omit-frame-pointer") 
  add_compile_options("-fno-optimize-sibling-calls")
  add_compile_options("-fsanitize=address")
  add_compile_options("-fsanitize-address-use-after-scope")
  set(CMAKE_EXE_LINKER_FLAGS "-fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address") 
endif()

option(ENABLE_SANITIZE_THREAD "enable thread sanitize" OFF)

if(ENABLE_SANITIZE_THREAD) 
  add_compile_options("-fno-omit-frame-pointer") 
  add_compile_options("-fno-optimize-sibling-calls")
  add_compile_options("-fsanitize=thread")
  add_compile_options("-fsanitize-address-use-after-scope")
  set(CMAKE_EXE_LINKER_FLAGS "-fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=thread") 
endif()

set(CMAKE_CXX_CLANG_TIDY
  clang-tidy;
  -format-style='file';
  -header-filter=${CMAKE_CURRENT_SOURCE_DIR};
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
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

add_library(proxy STATIC 
	${SOURCES}
)

target_link_libraries(proxy
		PRIVATE asio::asio
		PRIVATE spdlog::spdlog
)

add_executable(server main.cc)

target_link_libraries(
		${PROJECT_NAME}
		PRIVATE Threads::Threads
		PRIVATE spdlog::spdlog
		PRIVATE proxy
)