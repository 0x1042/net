cmake_minimum_required(VERSION 3.30)
project(sserver C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)

add_executable(${PROJECT_NAME} main.c
               server.c
               server.h
               log.c
               log.h)
