#include "command_handler.hpp"
#include "resp_parser.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unordered_map>
#include <unistd.h>

namespace {
    constexpr int MAX_EVENTS = 64;

    struct Client {
        std::string input_buffer;
        std::string output_buffer;
    };

    bool set_nonblocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);

        if (flags == -1) {
            return false;
        }

        return fcntl(
            fd,
            F_SETFL,
            flags | O_NONBLOCK
        ) != -1;
    }

    bool update_events(
        int epoll_fd,
        int fd,
        uint32_t events
    ) {
        epoll_event event{};
        event.events = events;
        event.data.fd = fd;

        return epoll_ctl(
            epoll_fd,
            EPOLL_CTL_MOD,
            fd,
            &event
        ) != -1;
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    if (!set_nonblocking(server_fd)) {
        perror("fcntl");
        close(server_fd);
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(6379);

    if (bind(
            server_fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }

    epoll_event server_event{};
    server_event.events = EPOLLIN;
    server_event.data.fd = server_fd;

    if (epoll_ctl(
            epoll_fd,
            EPOLL_CTL_ADD,
            server_fd,
            &server_event
        ) == -1) {
        perror("epoll_ctl");
        close(epoll_fd);
        close(server_fd);
        return 1;
    }

    epoll_event events[MAX_EVENTS];

    std::unordered_map<int, Client> clients;

    std::cout << "Redix listening on 127.0.0.1:6379\n";

    while (true) {
        int event_count = epoll_wait(
            epoll_fd,
            events,
            MAX_EVENTS,
            -1
        );

        if (event_count == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < event_count; ++i) {
            int fd = events[i].data.fd;
            uint32_t event_flags = events[i].events;

            // --------------------------------------------------
            // New client connection
            // --------------------------------------------------

            if (fd == server_fd) {
                while (true) {
                    int client_fd = accept(
                        server_fd,
                        nullptr,
                        nullptr
                    );

                    if (client_fd == -1) {
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK) {
                            break;
                        }

                        perror("accept");
                        break;
                    }

                    if (!set_nonblocking(client_fd)) {
                        perror("fcntl");
                        close(client_fd);
                        continue;
                    }

                    epoll_event client_event{};
                    client_event.events = EPOLLIN;
                    client_event.data.fd = client_fd;

                    if (epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_ADD,
                            client_fd,
                            &client_event
                        ) == -1) {
                        perror("epoll_ctl");
                        close(client_fd);
                        continue;
                    }

                    clients.emplace(
                        client_fd,
                        Client{}
                    );

                    std::cout << "Client connected\n";
                }

                continue;
            }

            // --------------------------------------------------
            // Client error / hangup
            // --------------------------------------------------

            if (event_flags & (EPOLLERR | EPOLLHUP)) {
                epoll_ctl(
                    epoll_fd,
                    EPOLL_CTL_DEL,
                    fd,
                    nullptr
                );

                clients.erase(fd);
                close(fd);

                std::cout << "Client disconnected\n";

                continue;
            }

            auto client_it = clients.find(fd);

            if (client_it == clients.end()) {
                continue;
            }

            Client& client = client_it->second;

            // --------------------------------------------------
            // Read from client
            // --------------------------------------------------

            if (event_flags & EPOLLIN) {
                char buffer[1024];

                while (true) {
                    ssize_t bytes_received = recv(
                        fd,
                        buffer,
                        sizeof(buffer),
                        0
                    );

                    if (bytes_received > 0) {
                        client.input_buffer.append(
                            buffer,
                            bytes_received
                        );

                        continue;
                    }

                    if (bytes_received == 0) {
                        epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_DEL,
                            fd,
                            nullptr
                        );

                        clients.erase(fd);
                        close(fd);

                        std::cout << "Client disconnected\n";

                        break;
                    }

                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK) {
                        break;
                    }

                    perror("recv");

                    epoll_ctl(
                        epoll_fd,
                        EPOLL_CTL_DEL,
                        fd,
                        nullptr
                    );

                    clients.erase(fd);
                    close(fd);

                    break;
                }

                // Client may have disconnected above.
                if (clients.find(fd) == clients.end()) {
                    continue;
                }

                // --------------------------------------------------
                // Parse complete commands
                // --------------------------------------------------

                while (true) {
                    ParseResult result =
                        parse_command(client.input_buffer);

                    if (result.status ==
                        ParseStatus::Incomplete) {
                        break;
                    }

                    if (result.status ==
                        ParseStatus::Invalid) {

                        client.output_buffer +=
                            "-ERR invalid request\r\n";

                        client.input_buffer.clear();

                        break;
                    }

                    std::string response =
                        handle_command(result.args);

                    client.output_buffer += response;

                    client.input_buffer.erase(
                        0,
                        result.bytes_consumed
                    );
                }

                // If there is something to send,
                // ask epoll to notify us when the socket
                // is writable.
                if (!client.output_buffer.empty()) {
                    if (!update_events(
                            epoll_fd,
                            fd,
                            EPOLLIN | EPOLLOUT
                        )) {
                        perror("epoll_ctl");
                    }
                }
            }

            // --------------------------------------------------
            // Write to client
            // --------------------------------------------------

            if (event_flags & EPOLLOUT) {
                while (!client.output_buffer.empty()) {
                    ssize_t bytes_sent = send(
                        fd,
                        client.output_buffer.data(),
                        client.output_buffer.size(),
                        0
                    );

                    if (bytes_sent > 0) {
                        client.output_buffer.erase(
                            0,
                            bytes_sent
                        );

                        continue;
                    }

                    if (bytes_sent == -1 &&
                        (errno == EAGAIN ||
                         errno == EWOULDBLOCK)) {
                        break;
                    }

                    if (bytes_sent == -1) {
                        perror("send");

                        epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_DEL,
                            fd,
                            nullptr
                        );

                        clients.erase(fd);
                        close(fd);

                        break;
                    }
                }

                // Client may have been closed above.
                if (clients.find(fd) == clients.end()) {
                    continue;
                }

                // Nothing left to write.
                // Stop watching EPOLLOUT.
                if (client.output_buffer.empty()) {
                    if (!update_events(
                            epoll_fd,
                            fd,
                            EPOLLIN
                        )) {
                        perror("epoll_ctl");
                    }
                }
            }
        }
    }

    close(epoll_fd);
    close(server_fd);

    return 0;
}