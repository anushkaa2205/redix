
#pragma once

#include <string>
#include <vector>

enum class ParseStatus {
    Complete,
    Incomplete,
    Invalid
};

struct ParseResult {
    ParseStatus status;
    std::vector<std::string> args;
    std::size_t bytes_consumed = 0;
};

ParseResult parse_command(const std::string& input);
