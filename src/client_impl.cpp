// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include "client_impl.hpp"

#include "detail/http_syntax.hpp"
#include "detail/multipart_request.hpp"
#include "detail/request_input.hpp"
#include "detail/transfer_state.hpp"

#include <kaycxx/core/scope.hpp>
#include <kaycxx/http/error.hpp>
#include <kaycxx/http/multipart_form.hpp>
#include <kaycxx/http/response_error.hpp>
#include <kaycxx/http/url_encoded_form.hpp>

#include <curl/curl.h>

#include <cstddef>
#include <exception>
#include <functional>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {

/**
 * Appends an application/x-www-form-urlencoded component.
 *
 * @param target  Target string.
 * @param value   Component bytes to encode.
 */
void append_url_encoded(std::string& target, std::string_view value) {
    constexpr auto hex_digits = std::string_view{"0123456789ABCDEF"};
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        const auto unescaped = (byte >= 'A' && byte <= 'Z')
            || (byte >= 'a' && byte <= 'z')
            || (byte >= '0' && byte <= '9')
            || byte == '*'
            || byte == '-'
            || byte == '.'
            || byte == '_';
        if (unescaped) {
            target.push_back(static_cast<char>(byte));
        } else if (byte == ' ') {
            target.push_back('+');
        } else {
            target.push_back('%');
            target.push_back(hex_digits[byte >> 4]);
            target.push_back(hex_digits[byte & 0x0f]);
        }
    }
}

} // namespace

namespace kaycxx::http {

using namespace detail;

client::impl::impl(request_options default_options)
    : curl_{nullptr, &curl_easy_cleanup},
      default_options_{std::move(default_options)} {
    ensure_curl_initialized();
    curl_.reset(curl_easy_init());
    if (!curl_) {
        throw error{"Unable to create libcurl easy handle"};
    }
    if (!default_options_.max_buffered_response_size.has_value()) {
        default_options_.max_buffered_response_size = 64 * 1024 * 1024;
    }
}

request_options client::impl::merge_options(const request_options& options) const {
    auto result = default_options_;
    std::erase_if(result.headers, [&options](const auto& default_header) {
        return has_header(options.headers, default_header.name);
    });
    result.headers.insert(result.headers.end(), options.headers.begin(), options.headers.end());

    if (options.follow_redirects.has_value()) {
        result.follow_redirects = options.follow_redirects;
    }
    if (options.max_redirects.has_value()) {
        result.max_redirects = options.max_redirects;
    }
    if (options.connect_timeout.has_value()) {
        result.connect_timeout = options.connect_timeout;
    }
    if (options.request_timeout.has_value()) {
        result.request_timeout = options.request_timeout;
    }
    if (options.progress) {
        result.progress = options.progress;
    }
    if (options.max_buffered_response_size.has_value()) {
        result.max_buffered_response_size = options.max_buffered_response_size;
    }
    return result;
}

buffered_response client::impl::perform(
    std::string_view method,
    std::string_view url,
    const request_body& body,
    const request_options& options
) {
    auto result = transfer(method, url, body, options, nullptr);
    return buffered_response{result.status_code, std::move(result.body), std::move(result.headers)};
}

streamed_response client::impl::perform(
    std::string_view method,
    std::string_view url,
    const request_body& body,
    std::ostream& output,
    const request_options& options
) {
    auto result = transfer(method, url, body, options, &output);
    return streamed_response{result.status_code, std::move(result.headers)};
}

std::string client::impl::serialize(const url_encoded_form& form) {
    auto result = std::string{};
    auto first = true;
    for (const auto& [name, value] : form.fields_) {
        if (!first) {
            result.push_back('&');
        }
        first = false;
        append_url_encoded(result, name);
        result.push_back('=');
        append_url_encoded(result, value);
    }
    return result;
}

void client::impl::prepare_request_input(const request_body& body, prepared_request_input& target) {
    target.has_body = !std::holds_alternative<std::monostate>(body.value_);
    std::visit([&](const auto& value) {
        using value_type = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<value_type, std::monostate>) {
            return;
        } else if constexpr (std::is_same_v<value_type, std::string_view>) {
            target.input.memory = std::span<const char>{value.data(), value.size()};
            target.size = value.size();
            target.default_content_type = "text/plain; charset=utf-8";
        } else if constexpr (std::is_same_v<value_type, std::span<const std::byte>>) {
            target.input.memory = std::span<const char>{reinterpret_cast<const char*>(value.data()), value.size()};
            target.size = value.size();
            target.default_content_type = "application/octet-stream";
        } else if constexpr (std::is_same_v<value_type, request_body::stream_data>) {
            target.input.stream = &value.input.get();
            target.input.start = current_position(*target.input.stream);
            if (value.size.has_value()) {
                target.size = *value.size;
            }
            target.default_content_type = "application/octet-stream";
        } else if constexpr (std::is_same_v<value_type, std::reference_wrapper<const nlohmann::json>>) {
            target.storage = value.get().dump();
            target.input.memory = std::span<const char>{target.storage.data(), target.storage.size()};
            target.size = target.storage.size();
            target.default_content_type = "application/json";
        } else if constexpr (std::is_same_v<value_type, std::reference_wrapper<const url_encoded_form>>) {
            target.storage = serialize(value.get());
            target.input.memory = std::span<const char>{target.storage.data(), target.storage.size()};
            target.size = target.storage.size();
            target.default_content_type = "application/x-www-form-urlencoded";
        } else if constexpr (std::is_same_v<value_type, std::reference_wrapper<const multipart_form>>) {
            throw error{"Nested multipart request bodies are not supported"};
        } else {
            static_assert(false, "Unhandled HTTP request body type");
        }
    }, body.value_);
}

void client::impl::add_multipart_part(multipart_request& request, const multipart_part& source) {
    auto* const part = curl_mime_addpart(request.get());
    if (part == nullptr) {
        throw error{"Unable to allocate HTTP multipart part"};
    }
    check(curl_mime_name(part, source.name.c_str()), "multipart part name");

    auto& input = request.add_input();
    prepare_request_input(source.body, input);
    const auto size = !input.has_body
        ? curl_off_t{0}
        : input.size.has_value()
            ? to_curl_size(*input.size, "multipart part body")
            : curl_off_t{-1};
    check(curl_mime_data_cb(part, size, &read_body, &seek_body, nullptr, &input.input), "multipart part body");

    if (source.filename.has_value()) {
        check(curl_mime_filename(part, source.filename->c_str()), "multipart part filename");
    }

    if (!has_header(source.headers, "Content-Type")) {
        const auto content_type = source.content_type.has_value() ? std::string_view{*source.content_type} : input.default_content_type;
        if (!content_type.empty()) {
            validate_header_value(content_type, "HTTP multipart part content type");
            const auto value = std::string{content_type};
            check(curl_mime_type(part, value.c_str()), "multipart part content type");
        }
    }

    auto headers = curl_headers{nullptr, &curl_slist_free_all};
    append_headers(headers, source.headers, header_context::multipart_part);
    if (headers != nullptr) {
        check(curl_mime_headers(part, headers.get(), 1), "multipart part headers");
        headers.release();
    }
}

transfer_result client::impl::transfer(
    std::string_view method,
    std::string_view url,
    const request_body& body,
    const request_options& options,
    std::ostream* output
) {
    if (in_progress_) {
        throw error{"HTTP client request already in progress"};
    }
    in_progress_ = true;
    const auto finish_transfer = kaycxx::core::scope_exit{[this]() noexcept {
        in_progress_ = false;
    }};
    const auto effective_options = merge_options(options);

    validate_http_token(method, "HTTP request method");

    auto* const handle = curl_.get();
    curl_easy_reset(handle);

    const auto request_method = std::string{method};
    const auto request_url = std::string{url};
    auto request_data = prepared_request_input{};
    auto multipart_data = std::optional<multipart_request>{};
    const auto* const multipart_value = std::get_if<std::reference_wrapper<const multipart_form>>(&body.value_);
    if (multipart_value == nullptr) {
        prepare_request_input(body, request_data);
    } else {
        if (multipart_value->get().parts_.empty()) {
            throw error{"HTTP multipart form must contain at least one part"};
        }
        multipart_data.emplace(handle);
        for (const auto& part : multipart_value->get().parts_) {
            add_multipart_part(*multipart_data, part);
        }
    }
    const auto has_body = multipart_data.has_value() || request_data.has_body;

    if (method == "HEAD" && has_body) {
        throw error{"HTTP HEAD request bodies are not supported"};
    }

    auto curl_body_size = std::optional<curl_off_t>{};
    if (request_data.size.has_value()) {
        curl_body_size = to_curl_size(*request_data.size, "request body");
    }

    auto response_data = response_state{
        .handle = handle,
        .stream = output,
        .max_buffered_response_size = effective_options.max_buffered_response_size
    };
    auto progress_data = progress_state{effective_options.progress};

    auto headers = curl_headers{nullptr, &curl_slist_free_all};
    append_headers(headers, effective_options.headers, multipart_data.has_value() ? header_context::multipart_request : header_context::request);
    if (!request_data.default_content_type.empty() && !has_header(effective_options.headers, "Content-Type")) {
        const auto content_type = std::string{"Content-Type: "} + std::string{request_data.default_content_type};
        append_header(headers, content_type);
    }

    check(curl_easy_setopt(handle, CURLOPT_URL, request_url.c_str()), "URL");
    if (headers != nullptr) {
        check(curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers.get()), "headers");
    }
    check(curl_easy_setopt(handle, CURLOPT_PROTOCOLS_STR, "http,https"), "protocols");
    if (effective_options.follow_redirects.has_value()) {
        check(curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, *effective_options.follow_redirects ? CURLFOLLOW_OBEYCODE : 0L), "redirect handling");
    }
    if (effective_options.max_redirects.has_value()) {
        check(curl_easy_setopt(handle, CURLOPT_MAXREDIRS, to_curl_long(*effective_options.max_redirects, "redirect limit")), "redirect limit");
    }
    if (effective_options.connect_timeout.has_value()) {
        const auto timeout = to_curl_long(effective_options.connect_timeout->count(), "connection timeout");
        check(curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, timeout), "connection timeout");
    }
    if (effective_options.request_timeout.has_value()) {
        check(curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, to_curl_long(effective_options.request_timeout->count(), "request timeout")), "request timeout");
    }
    check(curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L), "signal handling");
    check(curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, ""), "content decoding");
    check(curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, error_buffer_.data()), "error buffer");
    check(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &write_body), "response callback");
    check(curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response_data), "response output");

    if (effective_options.progress) {
        check(curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, &report_progress), "progress callback");
        check(curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &progress_data), "progress data");
        check(curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L), "progress reporting");
    }

    // Configure the closest native libcurl request mode before overriding its method below.
    auto native_method = std::string_view{};
    if (multipart_data.has_value()) {
        check(curl_easy_setopt(handle, CURLOPT_MIMEPOST, multipart_data->get()), "multipart form");
        native_method = "POST";
    } else if (request_data.has_body) {
        check(curl_easy_setopt(handle, CURLOPT_READFUNCTION, &read_body), "request callback");
        check(curl_easy_setopt(handle, CURLOPT_READDATA, &request_data.input), "request input");
        check(curl_easy_setopt(handle, CURLOPT_SEEKFUNCTION, &seek_body), "request seek callback");
        check(curl_easy_setopt(handle, CURLOPT_SEEKDATA, &request_data.input), "request seek data");

        if (method == "POST") {
            check(curl_easy_setopt(handle, CURLOPT_POST, 1L), "POST method");
            native_method = "POST";
            if (curl_body_size.has_value()) {
                check(curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE_LARGE, *curl_body_size), "request body size");
            }
        } else {
            check(curl_easy_setopt(handle, CURLOPT_UPLOAD, 1L), "request upload");
            native_method = "PUT";
            if (curl_body_size.has_value()) {
                check(curl_easy_setopt(handle, CURLOPT_INFILESIZE_LARGE, *curl_body_size), "request body size");
            }
        }
    } else if (method == "GET") {
        check(curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L), "GET method");
        native_method = "GET";
    } else if (method == "HEAD") {
        check(curl_easy_setopt(handle, CURLOPT_NOBODY, 1L), "HEAD method");
        native_method = "HEAD";
    } else if (method == "POST") {
        check(curl_easy_setopt(handle, CURLOPT_POST, 1L), "POST method");
        check(curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t{0}), "request body size");
        native_method = "POST";
    } else {
        check(curl_easy_setopt(handle, CURLOPT_UPLOAD, 1L), "request upload");
        check(curl_easy_setopt(handle, CURLOPT_INFILESIZE_LARGE, curl_off_t{0}), "request body size");
        native_method = "PUT";
    }

    if (method != native_method) {
        check(curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, request_method.c_str()), "request method");
        if (multipart_data.has_value()) {
            // Preserve custom multipart methods across 301/302 redirects, but allow 303 to switch to GET.
            const long post_redirects = CURL_REDIR_POST_301 | CURL_REDIR_POST_302;
            check(curl_easy_setopt(handle, CURLOPT_POSTREDIR, post_redirects), "redirect method handling");
        }
    }

    const auto curl_result = curl_easy_perform(handle);
    if (request_data.input.exception) {
        std::rethrow_exception(request_data.input.exception);
    }
    if (multipart_data.has_value()) {
        multipart_data->rethrow_input_exception();
    }
    if (response_data.exception) {
        std::rethrow_exception(response_data.exception);
    }
    if (progress_data.exception) {
        std::rethrow_exception(progress_data.exception);
    }
    if (response_data.status_result != CURLE_OK) {
        throw error{std::string{"Unable to read HTTP response status: "} + curl_easy_strerror(response_data.status_result)};
    }
    if (curl_result != CURLE_OK) {
        const auto detail = error_buffer_.front() == '\0' ? std::string{curl_easy_strerror(curl_result)} : std::string{error_buffer_.data()};
        throw error{request_method + " request failed: " + detail};
    }

    long raw_status_code = 0;
    check(curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &raw_status_code), "response status");
    const auto status_code = static_cast<int>(raw_status_code);
    auto response_headers = read_response_headers(handle);
    if (status_code < 200 || status_code >= 300) {
        throw response_error{
            request_method + " request failed: HTTP status " + std::to_string(status_code),
            buffered_response{status_code, std::move(response_data.body), std::move(response_headers)}
        };
    }
    return transfer_result{status_code, std::move(response_data.body), std::move(response_headers)};
}

} // namespace kaycxx::http
