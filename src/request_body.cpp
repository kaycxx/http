// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/request_body.hpp>

#include <kaycxx/http/multipart_form.hpp>
#include <kaycxx/http/url_encoded_form.hpp>

#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace kaycxx::http {

request_body::request_body() noexcept = default;

request_body::request_body(std::string_view body) noexcept
    : value_{body} {
}

request_body::request_body(const std::string& body) noexcept
    : request_body{std::string_view{body}} {
}

request_body::request_body(const char* body) {
    if (body == nullptr) {
        throw std::invalid_argument{"HTTP request body must not be null"};
    }
    value_ = std::string_view{body};
}

request_body::request_body(std::span<const std::byte> body) noexcept
    : value_{body} {
}

request_body::request_body(std::span<std::byte> body) noexcept
    : request_body{std::span<const std::byte>{body}} {
}

request_body::request_body(const std::vector<std::byte>& body) noexcept
    : request_body{std::span<const std::byte>{body}} {
}

request_body::request_body(std::istream& input) noexcept
    : value_{stream_data{std::ref(input), std::nullopt}} {
}

request_body::request_body(sized_istream input) noexcept
    : value_{stream_data{input.input_, input.size_}} {
}

request_body::request_body(const nlohmann::json& body) noexcept
    : value_{std::cref(body)} {
}

request_body::request_body(const url_encoded_form& body) noexcept
    : value_{std::cref(body)} {
}

request_body::request_body(const multipart_form& body) noexcept
    : value_{std::cref(body)} {
}

} // namespace kaycxx::http
