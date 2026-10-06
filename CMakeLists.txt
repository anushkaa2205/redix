cmake_minimum_required(VERSION 3.20)

project(Redix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(redix-server
    src/server.cpp
    src/command_handler.cpp
    src/resp_parser.cpp
)
add_executable(test-resp-parser
    tests/test_resp_parser.cpp
    src/resp_parser.cpp
)

target_include_directories(test-resp-parser PRIVATE src)