// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include "http_syntax.hpp"

#include <kaycxx/http/error.hpp>

#include <string>
#include <string_view>

namespace kaycxx::http::detail {

void validate_http_token(std::string_view value, std::string_view description) {
    constexpr auto punctuation = std::string_view{"!#$%&'*+-.^_`|~"};
    if (value.empty()) {
        throw error{std::string{"Invalid "} + std::string{description}};
    }
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        const auto valid = (byte >= 'A' && byte <= 'Z')
            || (byte >= 'a' && byte <= 'z')
            || (byte >= '0' && byte <= '9')
            || punctuation.contains(static_cast<char>(byte));
        if (!valid) {
            throw error{std::string{"Invalid "} + std::string{description}};
        }
    }
}

void validate_header_value(std::string_view value, std::string_view description) {
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if ((byte < 0x20 && byte != '\t') || byte == 0x7f) {
            throw error{std::string{description} + " contains an invalid control character"};
        }
    }
}

} // namespace kaycxx::http::detail
