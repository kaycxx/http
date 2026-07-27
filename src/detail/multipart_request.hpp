// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

/**
 * @file
 * Defines internal state for a libcurl multipart request.
 */

#include "curl_support.hpp"
#include "request_input.hpp"

#include <memory>
#include <vector>

namespace kaycxx::http::detail {

/** Request state for a multipart form transfer. */
class multipart_request {
public:
    /**
     * Creates an empty MIME tree for an easy handle.
     *
     * @param handle  libcurl easy handle.
     * @throws error  When the MIME tree cannot be allocated.
     */
    explicit multipart_request(CURL* handle);

    multipart_request(const multipart_request&) = delete;
    multipart_request& operator=(const multipart_request&) = delete;
    multipart_request(multipart_request&&) noexcept = default;
    multipart_request& operator=(multipart_request&&) noexcept = default;

    /**
     * Returns the underlying libcurl MIME handle.
     *
     * @returns libcurl MIME handle.
     */
    [[nodiscard]] curl_mime* get() const noexcept;

    /**
     * Adds stable storage for a multipart part input.
     *
     * @returns Added input storage.
     */
    prepared_request_input& add_input();

    /** Rethrows the first exception raised by a multipart part input callback. */
    void rethrow_input_exception() const;

private:
    /** Stable callback state for all multipart part inputs. */
    std::vector<std::unique_ptr<prepared_request_input>> inputs_{};

    /** MIME tree, declared after the inputs so it is destroyed before them. */
    curl_mime_handle mime_;
};

} // namespace kaycxx::http::detail
