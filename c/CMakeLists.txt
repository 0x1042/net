cmake_minimum_required(VERSION 3.30)
project(sserver C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)

option(ENABLE_TIDY "enable clang-tidy" OFF)

if(ENABLE_TIDY)
  find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
  mark_as_advanced(CLANG_TIDY_EXECUTABLE)
  if(${CLANG_TIDY_EXECUTABLE})
    message(STATUS "Clang-tidy not found")
  else()
    message(STATUS "Enabling clang-tidy")
    set(CMAKE_C_CLANG_TIDY
        "${CLANG_TIDY_EXECUTABLE};-format-style='file';-header-filter=${CMAKE_CURRENT_SOURCE_DIR}"
    )
  endif()
endif()

option(ENABLE_SANITIZE "enable address sanitize" OFF)
if(ENABLE_SANITIZE)
  add_compile_options("-fno-omit-frame-pointer")
  add_compile_options("-fno-optimize-sibling-calls")
  add_compile_options("-fsanitize=address")
  add_compile_options("-fsanitize-address-use-after-scope")
  set(CMAKE_EXE_LINKER_FLAGS
      "-fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address")
endif()

add_executable(
  ${PROJECT_NAME}
  main.c
  server.c
  server.h
  relay.h
  relay.c
  http.h
  http.c
  socks.h
  socks.c
  log.c
  log.h)
