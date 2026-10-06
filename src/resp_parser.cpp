#include "resp_parser.hpp"

#include <stdexcept>

ParseResult parse_command(const std::string& input) {
    ParseResult result;

    if (input.empty()) {
        result.status = ParseStatus::Incomplete;
        return result;
    }

    if (input[0] != '*') {
        result.status = ParseStatus::Invalid;
        return result;
    }

    std::size_t line_end = input.find("\r\n");

    if (line_end == std::string::npos) {
        result.status = ParseStatus::Incomplete;
        return result;
    }

    int arg_count;

    try {
        arg_count = std::stoi(input.substr(1, line_end - 1));
    } catch (...) {
        result.status = ParseStatus::Invalid;
        return result;
    }

    if (arg_count < 0) {
        result.status = ParseStatus::Invalid;
        return result;
    }

    std::size_t pos = line_end + 2;

    for (int i = 0; i < arg_count; ++i) {

        if (pos >= input.size()) {
            result.status = ParseStatus::Incomplete;
            return result;
        }

        if (input[pos] != '$') {
            result.status = ParseStatus::Invalid;
            return result;
        }

        line_end = input.find("\r\n", pos);

        if (line_end == std::string::npos) {
            result.status = ParseStatus::Incomplete;
            return result;
        }

        int length;

        try {
            length = std::stoi(
                input.substr(pos + 1, line_end - pos - 1)
            );
        } catch (...) {
            result.status = ParseStatus::Invalid;
            return result;
        }

        if (length < 0) {
            result.status = ParseStatus::Invalid;
            return result;
        }

        pos = line_end + 2;

        if (input.size() - pos <
            static_cast<std::size_t>(length + 2)) {
            result.status = ParseStatus::Incomplete;
            return result;
        }

        result.args.push_back(input.substr(pos, length));

        pos += length;

        if (input.compare(pos, 2, "\r\n") != 0) {
            result.status = ParseStatus::Invalid;
            return result;
        }

        pos += 2;
    }

    result.status = ParseStatus::Complete;
    result.bytes_consumed = pos;

    return result;
}