// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <kaycxx/http/header.hpp>
#include <kaycxx/http/transfer_progress.hpp>

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace kaycxx::http {

/**
 * Optional HTTP request configuration.
 *
 * Options passed to a request override the corresponding defaults configured on its client. Unset options inherit the client value or, when the client also
 * leaves them unset, the library or libcurl default.
 */
struct request_options {
    /**
     * Request headers.
     *
     * `Content-Length` and `Transfer-Encoding` are managed by the client. Multipart requests also manage their outer `Content-Type`.
     * Request-specific headers replace client headers with the same ASCII case-insensitive name and retain all other client headers.
     */
    std::vector<header> headers{};

    /** Whether HTTP redirects are followed. `std::nullopt` inherits the client setting. */
    std::optional<bool> follow_redirects{};

    /** Maximum number of redirects to follow. `std::nullopt` inherits the client setting. */
    std::optional<std::size_t> max_redirects{};

    /** Maximum duration of the connection phase. `std::nullopt` inherits the client setting. */
    std::optional<std::chrono::milliseconds> connect_timeout{};

    /** Maximum duration of the complete request. `std::nullopt` inherits the client setting. */
    std::optional<std::chrono::milliseconds> request_timeout{};

    /** Callback receiving transfer progress. An empty function inherits the client callback. */
    std::function<void(const transfer_progress&)> progress{};

    /** Maximum size of an internally buffered response body. `std::nullopt` inherits the client setting and zero disables the limit. */
    std::optional<std::size_t> max_buffered_response_size{};
};

} // namespace kaycxx::http
