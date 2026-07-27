// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

/**
 * @file
 * Declares internal HTTP transfer state and callbacks.
 */

#include <kaycxx/http/header.hpp>
#include <kaycxx/http/transfer_progress.hpp>

#include <curl/curl.h>

#include <cstddef>
#include <exception>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace kaycxx::http::detail {

/** Response body state used by the libcurl write callback. */
struct response_state {
    /** libcurl easy handle used to query the response status before streaming the body. */
    CURL* handle = nullptr;

    /** Optional output stream receiving a successful response body. */
    std::ostream* stream = nullptr;

    /** Maximum size of the internally buffered response body. `std::nullopt` or zero disables the limit. */
    std::optional<std::size_t> max_buffered_response_size{};

    /** Buffered response body when no output stream is used or the response status is unsuccessful. */
    std::string body{};

    /** Final HTTP response status code, queried when the first body data is received. */
    int status_code = 0;

    /** Result of querying the response status from within the write callback. */
    CURLcode status_result = CURLE_OK;

    /** Exception thrown while writing the response body. */
    std::exception_ptr exception{};
};

/** State used by the libcurl transfer progress callback. */
struct progress_state {
    /** Callback receiving transfer progress. */
    const std::function<void(const transfer_progress&)>& callback;

    /** Exception thrown by the progress callback. */
    std::exception_ptr exception{};
};

/** Completed successful HTTP transfer. */
struct transfer_result {
    /** HTTP status code in the range 200 to 299. */
    int status_code;

    /** Buffered response body, or an empty string when it was streamed. */
    std::string body;

    /** Response headers in their received order. */
    std::vector<header> headers;
};

/**
 * Writes received response body data to a buffer or output stream.
 *
 * Successful response data is streamed when an output stream is configured. Unsuccessful response data is always buffered for the response error.
 *
 * @param data       Received data.
 * @param size       Size of each received element.
 * @param count      Number of received elements.
 * @param user_data  Response output state.
 * @returns Number of consumed bytes, or `CURL_WRITEFUNC_ERROR` when writing failed.
 */
std::size_t write_body(char* data, std::size_t size, std::size_t count, void* user_data) noexcept;

/**
 * Reports transfer progress received from libcurl.
 *
 * @param user_data       Transfer progress state.
 * @param download_total  Expected number of downloaded bytes, or zero when unknown.
 * @param downloaded      Number of bytes downloaded so far.
 * @param upload_total    Expected number of uploaded bytes, or zero when unknown.
 * @param uploaded        Number of bytes uploaded so far.
 * @returns `0` to continue the transfer or `1` to abort after a callback threw an exception.
 */
int report_progress(
    void* user_data,
    curl_off_t download_total,
    curl_off_t downloaded,
    curl_off_t upload_total,
    curl_off_t uploaded
) noexcept;

} // namespace kaycxx::http::detail
