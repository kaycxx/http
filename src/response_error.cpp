// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/response_error.hpp>

#include <utility>

namespace kaycxx::http {

response_error::response_error(std::string message, buffered_response response)
    : error{std::move(message)}, response_{std::move(response)} {
}

response_error::~response_error() = default;

const buffered_response& response_error::response() const & noexcept {
    return response_;
}

buffered_response response_error::response() && noexcept {
    return std::move(response_);
}

} // namespace kaycxx::http
