cmake_minimum_required(VERSION 3.26)
project(server LANGUAGES C CXX ASM)

include(CheckCXXCompilerFlag)
include(CMakeDependentOption)
include(CheckLanguage)
include(GNUInstallDirs)
include(FetchContent)

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

check_cxx_compiler_flag("-std=c++20" COMPILER_SUPPORTS_CXX20)
if (COMPILER_SUPPORTS_CXX20)
	message(STATUS "Compiler supports C++20")
else ()
	message(WARNING "Compiler does not support C++20")
endif ()

add_compile_options(-Wall -Wextra -pedantic -g)
add_compile_options(-fsized-deallocation)
add_compile_options(-Werror=delete-non-virtual-dtor)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL
    "AppleClang")
	message(STATUS "Using Clang")
	add_compile_options(-fexperimental-library)
	message(STATUS "Enabling split-dwarf build")
	add_compile_options(-gsplit-dwarf=single)
	add_link_options(-gsplit-dwarf)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

option(ENABLE_SANITIZE "enable address sanitize" ON)
if (ENABLE_SANITIZE)
	add_compile_options("-fno-omit-frame-pointer")
	add_compile_options("-fno-optimize-sibling-calls")
	add_compile_options("-fsanitize=address")
	add_compile_options("-fsanitize-address-use-after-scope")
	set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
endif ()

option(ENABLE_SANITIZE_THREAD "enable thread sanitize" OFF)

if (ENABLE_SANITIZE_THREAD)
	add_compile_options("-fno-omit-frame-pointer")
	add_compile_options("-fno-optimize-sibling-calls")
	add_compile_options("-fsanitize=thread")
	add_compile_options("-fsanitize-address-use-after-scope")
	set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread")
endif ()

option(ENABLE_TIDY "enable clang-tidy" OFF)
if (ENABLE_TIDY)
	set(CMAKE_CXX_CLANG_TIDY clang-tidy; -format-style='file';
	    -header-filter=${CMAKE_CURRENT_SOURCE_DIR};)
endif ()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

find_package(PkgConfig)
find_package(Threads REQUIRED)

set(ASIO_NO_DEPRECATED OFF)
FetchContent_Declare(
		asiocmake
		GIT_REPOSITORY "https://github.com/OlivierLDff/asio.cmake"
		GIT_TAG v1.3.1)
FetchContent_MakeAvailable(asiocmake)

fetchcontent_declare(
		spdlog
		GIT_REPOSITORY https://github.com/gabime/spdlog
		GIT_TAG v1.15.3
)

FetchContent_MakeAvailable(spdlog)

add_subdirectory(${PROJECT_SOURCE_DIR}/lib)

add_executable(server main.cc)

target_link_libraries(
		${PROJECT_NAME}
		PRIVATE Threads::Threads
		PRIVATE spdlog::spdlog
		PRIVATE asio::asio
		PRIVATE proxy
)
