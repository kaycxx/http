// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/buffered_response.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kaycxx::http {

buffered_response::buffered_response(int status_code, std::string body, std::vector<kaycxx::http::header> headers)
    : response_base{status_code, std::move(headers)}, body_{std::move(body)} {
}

std::string_view buffered_response::text() const & noexcept {
    return body_;
}

std::string buffered_response::text() && noexcept {
    return std::move(body_);
}

nlohmann::json buffered_response::json() const {
    if (body_.empty()) {
        return nullptr;
    }

    return nlohmann::json::parse(body_);
}

} // namespace kaycxx::http
