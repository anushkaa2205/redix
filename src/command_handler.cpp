#include "command_handler.hpp"

std::string handle_command(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "-ERR empty command\r\n";
    }

    if (args[0] == "PING") {
        return "+PONG\r\n";
    }

    return "-ERR unknown command\r\n";
}