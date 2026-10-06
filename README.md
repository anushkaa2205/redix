# Redix

Redix is a Redis-compatible in-memory key-value server built from scratch in C++20.

## Why Redix?

The goal of this project is to understand how a high-performance key-value server works internally instead of treating Redis as a black box.

Redix focuses on learning and implementing core systems concepts such as:

- TCP networking
- Event-driven I/O with `epoll`
- RESP2 protocol parsing
- In-memory data structures
- Command processing
- TTL and expiration
- Persistence
- Testing and benchmarking

## Key Features

- Redis-compatible RESP2 protocol
- Event-driven, non-blocking TCP server
- Multiple client connections
- In-memory key-value storage
- Redis-style commands and data types
- Key expiration and TTL
- Persistence with AOF/RDB-style mechanisms
- Testing against Redis for compatibility

## Tech Stack

- C++20
- Linux
- CMake
- epoll
- TCP
- RESP2

## Build

```bash
cmake -S . -B build
cmake --build build
