// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/response_base.hpp>

#include "detail/ascii.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kaycxx::http {

response_base::response_base(int status_code, std::vector<kaycxx::http::header> headers)
    : status_code_{status_code}, headers_{std::move(headers)} {
}

int response_base::status_code() const noexcept {
    return status_code_;
}

const std::vector<kaycxx::http::header>& response_base::headers() const & noexcept {
    return headers_;
}

std::vector<kaycxx::http::header> response_base::headers() && noexcept {
    return std::move(headers_);
}

std::optional<std::string_view> response_base::header(std::string_view name) const & noexcept {
    for (const auto& response_header : headers_) {
        if (detail::equals_ignore_case(response_header.name, name)) {
            return response_header.value;
        }
    }
    return std::nullopt;
}

std::optional<std::string> response_base::header(std::string_view name) && noexcept {
    for (auto& response_header : headers_) {
        if (detail::equals_ignore_case(response_header.name, name)) {
            return std::move(response_header.value);
        }
    }
    return std::nullopt;
}

std::vector<std::string> response_base::headers(std::string_view name) const {
    auto values = std::vector<std::string>{};
    for (const auto& response_header : headers_) {
        if (detail::equals_ignore_case(response_header.name, name)) {
            values.push_back(response_header.value);
        }
    }
    return values;
}

} // namespace kaycxx::http
