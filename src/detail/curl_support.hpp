// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

/**
 * @file
 * Declares internal libcurl helpers.
 */

#include <kaycxx/http/error.hpp>
#include <kaycxx/http/header.hpp>

#include <curl/curl.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace kaycxx::http::detail {

/** Owning libcurl easy handle. */
using curl_handle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;

/** Owning libcurl request header list. */
using curl_headers = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;

/** Owning libcurl MIME handle. */
using curl_mime_handle = std::unique_ptr<curl_mime, decltype(&curl_mime_free)>;

/** Destination controlling how an empty header is represented for libcurl. */
enum class header_context {
    /** Complete HTTP request headers configured through `CURLOPT_HTTPHEADER`. */
    request,

    /** Complete multipart HTTP request headers configured through `CURLOPT_HTTPHEADER`. */
    multipart_request,

    /** Multipart part headers configured through `curl_mime_headers`. */
    multipart_part
};

/**
 * Initializes libcurl once for the process and registers its cleanup function.
 *
 * @throws error  When libcurl initialization or cleanup registration fails.
 */
void ensure_curl_initialized();

/**
 * Checks the result of a libcurl configuration operation.
 *
 * @param result     Result returned by libcurl.
 * @param operation  Human-readable operation name for the error message.
 * @throws error  When the operation failed.
 */
void check(CURLcode result, std::string_view operation);

/**
 * Converts an integral option value to the representation required by libcurl.
 *
 * @tparam value_type  Integral source type.
 * @param value        Value to convert.
 * @param operation    Human-readable option name for the error message.
 * @returns Converted value.
 * @throws error  When the value cannot be represented by `long`.
 */
template<typename value_type>
[[nodiscard]] long to_curl_long(value_type value, std::string_view operation) {
    static_assert(std::is_integral_v<value_type>);
    if (!std::in_range<long>(value)) {
        throw error{std::string{"HTTP "} + std::string{operation} + " is outside libcurl's supported range"};
    }
    return static_cast<long>(value);
}

/**
 * Converts a byte count to the representation required by libcurl.
 *
 * @param size         Byte count.
 * @param description  Human-readable value description for the error message.
 * @returns Converted byte count.
 * @throws error  When the byte count cannot be represented by `curl_off_t`.
 */
[[nodiscard]] curl_off_t to_curl_size(std::uintmax_t size, std::string_view description);

/**
 * Appends a request header to a libcurl header list.
 *
 * @param headers  Header list to modify.
 * @param value    Complete header value to append.
 * @throws error  When the header cannot be allocated.
 */
void append_header(curl_headers& headers, const std::string& value);

/**
 * Checks whether a header list contains a header with a specific name.
 *
 * @param headers  Headers to inspect.
 * @param name     Header name to find.
 * @returns True when a matching header is present.
 */
[[nodiscard]] bool has_header(std::span<const header> headers, std::string_view name) noexcept;

/**
 * Validates and appends configured request headers.
 *
 * @param target   Target libcurl header list.
 * @param headers  Headers to append.
 * @param context  Header destination.
 * @throws error  When a header is invalid or cannot be allocated.
 */
void append_headers(curl_headers& target, std::span<const header> headers, header_context context);

/**
 * Copies the final response headers stored and parsed by libcurl.
 *
 * @param handle  libcurl easy handle of the completed transfer.
 * @returns Final response headers and trailers in their received order.
 */
[[nodiscard]] std::vector<header> read_response_headers(CURL* handle);

} // namespace kaycxx::http::detail
