// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

/**
 * @file
 * Declares the private libcurl-backed HTTP client implementation.
 */

#include "detail/curl_support.hpp"

#include <kaycxx/http/client.hpp>

#include <array>
#include <iosfwd>
#include <string>
#include <string_view>

namespace kaycxx::http {

class url_encoded_form;
struct multipart_part;

namespace detail {

class multipart_request;
struct prepared_request_input;
struct transfer_result;

} // namespace detail

/** Private libcurl-backed implementation of the HTTP client. */
class client::impl {
public:
    /**
     * Creates a reusable libcurl easy handle.
     *
     * @throws error  When libcurl cannot be initialized or its easy handle cannot be created.
     */
    impl();

    /**
     * Performs an HTTP request and buffers the response body.
     *
     * @param method   HTTP request method.
     * @param url      Request URL.
     * @param body     Request body.
     * @param options  Request options.
     * @returns Successful HTTP response.
     */
    [[nodiscard]] buffered_response perform(
        std::string_view method,
        std::string_view url,
        const request_body& body,
        const request_options& options
    );

    /**
     * Performs an HTTP request and streams the successful response body.
     *
     * @param method   HTTP request method.
     * @param url      Request URL.
     * @param body     Request body.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     */
    [[nodiscard]] streamed_response perform(
        std::string_view method,
        std::string_view url,
        const request_body& body,
        std::ostream& output,
        const request_options& options
    );

private:
    /**
     * Serializes a URL-encoded form.
     *
     * @param form  Form to serialize.
     * @returns Serialized form body.
     */
    [[nodiscard]] static std::string serialize(const url_encoded_form& form);

    /**
     * Prepares a non-multipart request body for libcurl callbacks.
     *
     * @param body    Request body to prepare.
     * @param target  Target input state.
     * @throws error                       When a nested multipart form is encountered.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    static void prepare_request_input(const request_body& body, detail::prepared_request_input& target);

    /**
     * Adds a public multipart part to a libcurl MIME tree.
     *
     * @param request  Multipart request state.
     * @param source   Multipart part to add.
     * @throws error                       When the part cannot be configured.
     * @throws nlohmann::json::type_error  When a JSON part cannot be serialized.
     */
    static void add_multipart_part(detail::multipart_request& request, const multipart_part& source);

    /**
     * Performs a configured HTTP transfer.
     *
     * @param method   HTTP request method.
     * @param url      Request URL.
     * @param body     Request body.
     * @param options  Request options.
     * @param output   Optional output stream receiving the successful response body.
     * @returns Successful transfer result.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] detail::transfer_result transfer(
        std::string_view method,
        std::string_view url,
        const request_body& body,
        const request_options& options,
        std::ostream* output
    );

    /** libcurl error message buffer, declared before the handle so it is destroyed after the handle. */
    std::array<char, CURL_ERROR_SIZE> error_buffer_{};

    /** Reusable libcurl easy handle retaining its connection cache between requests. */
    detail::curl_handle curl_;

    /** Whether this client currently performs a request. */
    bool in_progress_ = false;
};

} // namespace kaycxx::http
