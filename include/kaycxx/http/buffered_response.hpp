// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <kaycxx/http/header.hpp>
#include <kaycxx/http/response_base.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace kaycxx::http {

/** HTTP response with a buffered body. */
class buffered_response : public response_base {
public:
    /**
     * Creates a buffered HTTP response.
     *
     * @param status_code  HTTP status code.
     * @param body         Response body.
     * @param headers      Response headers.
     */
    buffered_response(int status_code, std::string body, std::vector<kaycxx::http::header> headers = {});

    /**
     * Returns a non-owning view of the response body without interpreting its contents.
     *
     * The returned view remains valid until this response is destroyed, moved from or assigned a new value.
     *
     * @returns View of the response body.
     */
    [[nodiscard]] std::string_view text() const & noexcept;

    /**
     * Moves the response body out of an expiring response.
     *
     * @returns Response body.
     */
    [[nodiscard]] std::string text() && noexcept;

    /**
     * Parses and returns the response body as JSON.
     *
     * @returns Parsed JSON value. An empty response body is returned as JSON null.
     * @throws nlohmann::json::parse_error  When the response body does not contain valid JSON.
     */
    [[nodiscard]] nlohmann::json json() const;

private:
    /** Response body. */
    std::string body_;
};

} // namespace kaycxx::http
