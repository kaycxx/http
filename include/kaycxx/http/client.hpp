// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <kaycxx/http/buffered_response.hpp>
#include <kaycxx/http/request_body.hpp>
#include <kaycxx/http/request_options.hpp>
#include <kaycxx/http/response_body.hpp>
#include <kaycxx/http/streamed_response.hpp>

#include <memory>
#include <string_view>

namespace kaycxx::http {

/**
 * Synchronous HTTP client.
 *
 * A client retains its backend handle so sequential requests can reuse connections. A client instance must not be used concurrently from multiple threads.
 * Starting another request reentrantly on the same instance throws `error`.
 * A moved-from client can be destroyed or move-assigned; attempting to send a request with it throws `error`.
 */
class client {
public:
    /**
     * Creates an HTTP client.
     *
     * @throws error  When libcurl cannot be initialized or its easy handle cannot be created.
     */
    client();

    /** Destroys the HTTP client. */
    ~client();

    client(const client&) = delete;
    client& operator=(const client&) = delete;

    /**
     * Moves an HTTP client.
     *
     * @param other  HTTP client to move from.
     */
    client(client&& other) noexcept;

    /**
     * Move-assigns an HTTP client.
     *
     * @param other  HTTP client to move from.
     * @returns Reference to this HTTP client.
     */
    client& operator=(client&& other) noexcept;

    /**
     * Sends an HTTP request and buffers the response body.
     *
     * @param method   HTTP request method.
     * @param url      Request URL.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] buffered_response request(std::string_view method, std::string_view url, const request_options& options = {});

    /**
     * Sends an HTTP request with a body and buffers the response body.
     *
     * @param method   HTTP request method.
     * @param url      Request URL.
     * @param body     Request body.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] buffered_response request(
        std::string_view method,
        std::string_view url,
        request_body body,
        const request_options& options = {}
    );

    /**
     * Sends an HTTP request and streams the successful response body to an output stream.
     *
     * Error response bodies are retained by the thrown response error and are not written to the output stream. A transport or output error can leave a partial
     * successful response in the output stream.
     *
     * @param method   HTTP request method.
     * @param url      Request URL.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] streamed_response request(
        std::string_view method,
        std::string_view url,
        response_body output,
        const request_options& options = {}
    );

    /**
     * Sends an HTTP request with a body and streams the successful response body to an output stream.
     *
     * Error response bodies are retained by the thrown response error and are not written to the output stream. A transport or output error can leave a partial
     * successful response in the output stream.
     *
     * @param method   HTTP request method.
     * @param url      Request URL.
     * @param body     Request body.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] streamed_response request(
        std::string_view method,
        std::string_view url,
        request_body body,
        response_body output,
        const request_options& options = {}
    );

    /**
     * Sends a GET request and buffers the response body.
     *
     * @param url      Request URL.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] buffered_response get(std::string_view url, const request_options& options = {});

    /**
     * Sends a GET request with a body and buffers the response body.
     *
     * @param url      Request URL.
     * @param body     Request body.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] buffered_response get(std::string_view url, request_body body, const request_options& options = {});

    /**
     * Sends a GET request and streams the successful response body to an output stream.
     *
     * @param url      Request URL.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] streamed_response get(std::string_view url, response_body output, const request_options& options = {});

    /**
     * Sends a GET request with a body and streams the successful response body to an output stream.
     *
     * @param url      Request URL.
     * @param body     Request body.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] streamed_response get(
        std::string_view url,
        request_body body,
        response_body output,
        const request_options& options = {}
    );

    /**
     * Sends a POST request and buffers the response body.
     *
     * @param url      Request URL.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] buffered_response post(std::string_view url, const request_options& options = {});

    /**
     * Sends a POST request with a body and buffers the response body.
     *
     * @param url      Request URL.
     * @param body     Request body.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] buffered_response post(std::string_view url, request_body body, const request_options& options = {});

    /**
     * Sends a POST request and streams the successful response body to an output stream.
     *
     * @param url      Request URL.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] streamed_response post(std::string_view url, response_body output, const request_options& options = {});

    /**
     * Sends a POST request with a body and streams the successful response body to an output stream.
     *
     * @param url      Request URL.
     * @param body     Request body.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] streamed_response post(
        std::string_view url,
        request_body body,
        response_body output,
        const request_options& options = {}
    );

    /**
     * Sends a PUT request and buffers the response body.
     *
     * @param url      Request URL.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] buffered_response put(std::string_view url, const request_options& options = {});

    /**
     * Sends a PUT request with a body and buffers the response body.
     *
     * @param url      Request URL.
     * @param body     Request body.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] buffered_response put(std::string_view url, request_body body, const request_options& options = {});

    /**
     * Sends a PUT request and streams the successful response body to an output stream.
     *
     * @param url      Request URL.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] streamed_response put(std::string_view url, response_body output, const request_options& options = {});

    /**
     * Sends a PUT request with a body and streams the successful response body to an output stream.
     *
     * @param url      Request URL.
     * @param body     Request body.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] streamed_response put(
        std::string_view url,
        request_body body,
        response_body output,
        const request_options& options = {}
    );

    /**
     * Sends a DELETE request and buffers the response body.
     *
     * @param url      Request URL.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] buffered_response del(std::string_view url, const request_options& options = {});

    /**
     * Sends a DELETE request with a body and buffers the response body.
     *
     * @param url      Request URL.
     * @param body     Request body.
     * @param options  Request options.
     * @returns Successful HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] buffered_response del(std::string_view url, request_body body, const request_options& options = {});

    /**
     * Sends a DELETE request and streams the successful response body to an output stream.
     *
     * @param url      Request URL.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error  When the server returns a status code outside the range 200 to 299.
     * @throws error           When the request cannot be configured or performed.
     */
    [[nodiscard]] streamed_response del(std::string_view url, response_body output, const request_options& options = {});

    /**
     * Sends a DELETE request with a body and streams the successful response body to an output stream.
     *
     * @param url      Request URL.
     * @param body     Request body.
     * @param output   Output stream receiving the successful response body.
     * @param options  Request options.
     * @returns Successful streamed HTTP response.
     * @throws response_error              When the server returns a status code outside the range 200 to 299.
     * @throws error                       When the request cannot be configured or performed.
     * @throws nlohmann::json::type_error  When a JSON body cannot be serialized.
     */
    [[nodiscard]] streamed_response del(
        std::string_view url,
        request_body body,
        response_body output,
        const request_options& options = {}
    );

private:
    /** Private implementation type hiding all libcurl details. */
    class impl;

    /**
     * Returns the private client implementation.
     *
     * @returns Private client implementation.
     * @throws error  When this client has been moved from.
     */
    [[nodiscard]] impl& require_impl();

    /** Private client implementation. */
    std::unique_ptr<impl> impl_;
};

} // namespace kaycxx::http
