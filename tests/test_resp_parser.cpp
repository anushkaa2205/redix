#include "resp_parser.hpp"

#include <iostream>
#include <string>
#include <vector>

int main() {
    // 1. Complete request
    const std::string valid_input =
        "*3\r\n"
        "$3\r\nSET\r\n"
        "$4\r\nname\r\n"
        "$7\r\nAnushka\r\n";

    ParseResult result = parse_command(valid_input);

    if (result.status != ParseStatus::Complete) {
        std::cerr << "FAIL: complete request\n";
        return 1;
    }

    std::vector<std::string> expected = {
        "SET", "name", "Anushka"
    };

    if (result.args != expected) {
        std::cerr << "FAIL: arguments incorrect\n";
        return 1;
    }

    // 2. Incomplete request
    const std::string incomplete_input =
        "*1\r\n"
        "$4\r\nPI";

    result = parse_command(incomplete_input);

    if (result.status != ParseStatus::Incomplete) {
        std::cerr << "FAIL: incomplete request\n";
        return 1;
    }

    // 3. Invalid request
    const std::string invalid_input = "hello";

    result = parse_command(invalid_input);

    if (result.status != ParseStatus::Invalid) {
        std::cerr << "FAIL: invalid request\n";
        return 1;
    }

    std::cout << "PASS: complete request\n";
    std::cout << "PASS: incomplete request\n";
    std::cout << "PASS: invalid request\n";

    return 0;
}