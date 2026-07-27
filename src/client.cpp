// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/client.hpp>
#include <kaycxx/http/error.hpp>

#include "client_impl.hpp"

#include <memory>

namespace kaycxx::http {

client::client()
    : impl_{std::make_unique<impl>()} {
}

client::~client() = default;

client::client(client&& other) noexcept = default;

client& client::operator=(client&& other) noexcept = default;

client::impl& client::require_impl() {
    if (!impl_) {
        throw error{"HTTP client has been moved from"};
    }
    return *impl_;
}

buffered_response client::request(std::string_view method, std::string_view url, const request_options& options) {
    return require_impl().perform(method, url, request_body{}, options);
}

buffered_response client::request(std::string_view method, std::string_view url, request_body body, const request_options& options) {
    return require_impl().perform(method, url, body, options);
}

streamed_response client::request(std::string_view method, std::string_view url, response_body output, const request_options& options) {
    return require_impl().perform(method, url, request_body{}, output.output_.get(), options);
}

streamed_response client::request(
    std::string_view method,
    std::string_view url,
    request_body body,
    response_body output,
    const request_options& options
) {
    return require_impl().perform(method, url, body, output.output_.get(), options);
}

buffered_response client::get(std::string_view url, const request_options& options) {
    return request("GET", url, options);
}

buffered_response client::get(std::string_view url, request_body body, const request_options& options) {
    return request("GET", url, body, options);
}

streamed_response client::get(std::string_view url, response_body output, const request_options& options) {
    return request("GET", url, output, options);
}

streamed_response client::get(std::string_view url, request_body body, response_body output, const request_options& options) {
    return request("GET", url, body, output, options);
}

buffered_response client::post(std::string_view url, const request_options& options) {
    return request("POST", url, options);
}

buffered_response client::post(std::string_view url, request_body body, const request_options& options) {
    return request("POST", url, body, options);
}

streamed_response client::post(std::string_view url, response_body output, const request_options& options) {
    return request("POST", url, output, options);
}

streamed_response client::post(std::string_view url, request_body body, response_body output, const request_options& options) {
    return request("POST", url, body, output, options);
}

buffered_response client::put(std::string_view url, const request_options& options) {
    return request("PUT", url, options);
}

buffered_response client::put(std::string_view url, request_body body, const request_options& options) {
    return request("PUT", url, body, options);
}

streamed_response client::put(std::string_view url, response_body output, const request_options& options) {
    return request("PUT", url, output, options);
}

streamed_response client::put(std::string_view url, request_body body, response_body output, const request_options& options) {
    return request("PUT", url, body, output, options);
}

buffered_response client::del(std::string_view url, const request_options& options) {
    return request("DELETE", url, options);
}

buffered_response client::del(std::string_view url, request_body body, const request_options& options) {
    return request("DELETE", url, body, options);
}

streamed_response client::del(std::string_view url, response_body output, const request_options& options) {
    return request("DELETE", url, output, options);
}

streamed_response client::del(std::string_view url, request_body body, response_body output, const request_options& options) {
    return request("DELETE", url, body, output, options);
}

} // namespace kaycxx::http
