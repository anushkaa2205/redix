#include "command_handler.hpp"
#include "resp_parser.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(6379);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Redix listening on 127.0.0.1:6379\n";

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        std::string input_buffer;
        char buffer[1024];

        while (true) {
            ssize_t bytes_received = recv(
                client_fd,
                buffer,
                sizeof(buffer),
                0
            );

            if (bytes_received == 0) {
                // Client disconnected.
                break;
            }

            if (bytes_received < 0) {
                perror("recv");
                break;
            }

            input_buffer.append(buffer, bytes_received);

            // Process every complete command currently
            // stored in the input buffer.
            while (!input_buffer.empty()) {
                ParseResult result = parse_command(input_buffer);

                if (result.status == ParseStatus::Incomplete) {
                    // Keep the incomplete bytes.
                    break;
                }

                if (result.status == ParseStatus::Invalid) {
                    const char error[] = "-ERR invalid request\r\n";

                    send(
                        client_fd,
                        error,
                        sizeof(error) - 1,
                        0
                    );

                    input_buffer.clear();
                    break;
                }

                std::string response = handle_command(result.args);

                send(
                    client_fd,
                    response.c_str(),
                    response.size(),
                    0
                );

                input_buffer.erase(0, result.bytes_consumed);
            }
        }

        close(client_fd);
    }

    close(server_fd);

    return 0;
}