// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <kaycxx/http/header.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace kaycxx::http {

/** Common status and header data of an HTTP response. */
class response_base {
public:
    /**
     * Returns the HTTP status code.
     *
     * @returns HTTP status code.
     */
    [[nodiscard]] int status_code() const noexcept;

    /**
     * Returns all response headers in their received order.
     *
     * The returned reference remains valid until this response is destroyed, moved from or assigned a new value.
     *
     * @returns Response headers.
     */
    [[nodiscard]] const std::vector<kaycxx::http::header>& headers() const & noexcept;

    /**
     * Moves all response headers out of an expiring response.
     *
     * @returns Response headers.
     */
    [[nodiscard]] std::vector<kaycxx::http::header> headers() && noexcept;

    /**
     * Returns the first value of a response header using an ASCII case-insensitive name comparison.
     *
     * The returned view remains valid until this response is destroyed, moved from or assigned a new value.
     *
     * @param name  Header name.
     * @returns First matching header value, or an empty optional when the header is absent.
     */
    [[nodiscard]] std::optional<std::string_view> header(std::string_view name) const & noexcept;

    /**
     * Moves the first value of a response header out of an expiring response using an ASCII case-insensitive name comparison.
     *
     * @param name  Header name.
     * @returns First matching header value, or an empty optional when the header is absent.
     */
    [[nodiscard]] std::optional<std::string> header(std::string_view name) && noexcept;

    /**
     * Returns all values of a response header using an ASCII case-insensitive name comparison.
     *
     * @param name  Header name.
     * @returns Matching header values in their received order.
     */
    [[nodiscard]] std::vector<std::string> headers(std::string_view name) const;

protected:
    /**
     * Creates common HTTP response data.
     *
     * @param status_code  HTTP status code.
     * @param headers      Response headers.
     */
    response_base(int status_code, std::vector<kaycxx::http::header> headers);

    /** Copies common HTTP response data. */
    response_base(const response_base&) = default;

    /** Moves common HTTP response data. */
    response_base(response_base&&) noexcept = default;

    /** Copies common HTTP response data. */
    response_base& operator=(const response_base&) = default;

    /** Moves common HTTP response data. */
    response_base& operator=(response_base&&) noexcept = default;

    /** Destroys common HTTP response data. */
    ~response_base() = default;

private:
    /** HTTP status code. */
    int status_code_;

    /** Response headers in their received order. */
    std::vector<kaycxx::http::header> headers_;
};

} // namespace kaycxx::http
