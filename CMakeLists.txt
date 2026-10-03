cmake_minimum_required(VERSION 3.20)

project(Redix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(redix-server src/main.cpp)