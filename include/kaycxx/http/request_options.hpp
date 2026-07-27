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

/** Optional HTTP request configuration. */
struct request_options {
    /**
     * Request headers.
     *
     * `Content-Length` and `Transfer-Encoding` are managed by the client. Multipart requests also manage their outer `Content-Type`.
     */
    std::vector<header> headers{};

    /** Whether HTTP redirects are followed. `std::nullopt` uses the libcurl default. */
    std::optional<bool> follow_redirects{};

    /** Maximum number of redirects to follow. `std::nullopt` uses the libcurl default. */
    std::optional<std::size_t> max_redirects{};

    /** Maximum duration of the connection phase. `std::nullopt` uses the libcurl default. */
    std::optional<std::chrono::milliseconds> connect_timeout{};

    /** Maximum duration of the complete request. `std::nullopt` uses the libcurl default. */
    std::optional<std::chrono::milliseconds> request_timeout{};

    /** Callback receiving transfer progress. An empty function disables progress reporting. */
    std::function<void(const transfer_progress&)> progress{};

    /** Maximum size of an internally buffered response body. `std::nullopt` disables the limit. */
    std::optional<std::size_t> max_buffered_response_size{64 * 1024 * 1024};
};

} // namespace kaycxx::http
