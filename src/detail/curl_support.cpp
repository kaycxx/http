// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include "curl_support.hpp"

#include "ascii.hpp"
#include "http_syntax.hpp"

#include <cstdlib>
#include <limits>
#include <string>

namespace kaycxx::http::detail {

namespace {

/**
 * Validates that a header does not override request structure managed by the client.
 *
 * @param name     Header name.
 * @param context  Header destination.
 * @throws error  When the header is managed by the client in the specified context.
 */
void validate_header_context(std::string_view name, header_context context) {
    if (context != header_context::multipart_part
            && (equals_ignore_case(name, "Content-Length")
                || equals_ignore_case(name, "Transfer-Encoding"))) {
        throw error{std::string{"HTTP request "} + std::string{name} + " header is managed by the client"};
    }
    if (context == header_context::multipart_request && equals_ignore_case(name, "Content-Type")) {
        throw error{"HTTP multipart request Content-Type header is managed by the client"};
    }
    if (context == header_context::multipart_part && equals_ignore_case(name, "Content-Disposition")) {
        throw error{"HTTP multipart part Content-Disposition header is managed by the client"};
    }
}

} // namespace

void ensure_curl_initialized() {
    [[maybe_unused]] static const auto initialized = [] {
        const auto result = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (result != CURLE_OK) {
            throw error{std::string{"Unable to initialize libcurl: "} + curl_easy_strerror(result)};
        }
        if (std::atexit(&curl_global_cleanup) != 0) {
            curl_global_cleanup();
            throw error{"Unable to register libcurl cleanup"};
        }
        return true;
    }();
}

void check(CURLcode result, std::string_view operation) {
    if (result != CURLE_OK) {
        throw error{std::string{"Unable to configure HTTP "} + std::string{operation} + ": " + curl_easy_strerror(result)};
    }
}

curl_off_t to_curl_size(std::uintmax_t size, std::string_view description) {
    if (size > static_cast<std::uintmax_t>(std::numeric_limits<curl_off_t>::max())) {
        throw error{std::string{"HTTP "} + std::string{description} + " is too large for libcurl"};
    }
    return static_cast<curl_off_t>(size);
}

void append_header(curl_headers& headers, const std::string& value) {
    auto* appended = curl_slist_append(headers.get(), value.c_str());
    if (appended == nullptr) {
        throw error{"Unable to allocate HTTP request header"};
    }
    headers.release();
    headers.reset(appended);
}

bool has_header(std::span<const header> headers, std::string_view name) noexcept {
    for (const auto& request_header : headers) {
        if (equals_ignore_case(request_header.name, name)) {
            return true;
        }
    }
    return false;
}

void append_headers(curl_headers& target, std::span<const header> headers, header_context context) {
    for (const auto& request_header : headers) {
        const auto multipart_part = context == header_context::multipart_part;
        validate_http_token(request_header.name, multipart_part ? "HTTP multipart part header name" : "HTTP request header name");
        validate_header_value(request_header.value, multipart_part ? "HTTP multipart part header value" : "HTTP request header value");
        validate_header_context(request_header.name, context);

        const auto value = request_header.value.empty() ? request_header.name + (multipart_part ? ":" : ";")
                                                        : request_header.name + ": " + request_header.value;
        append_header(target, value);
    }
}

std::vector<header> read_response_headers(CURL* handle) {
    auto headers = std::vector<header>{};
    auto* previous = static_cast<curl_header*>(nullptr);
    constexpr auto origins = CURLH_HEADER | CURLH_TRAILER;
    while (auto* current = curl_easy_nextheader(handle, origins, -1, previous)) {
        headers.push_back(header{current->name, current->value});
        previous = current;
    }
    return headers;
}

} // namespace kaycxx::http::detail
