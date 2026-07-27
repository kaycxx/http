// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <httplib.h>

#include <string>
#include <string_view>
#include <thread>

namespace kaycxx::http::test {

/**
 * Local HTTP server providing endpoints for client tests.
 */
class server {
public:
    /** Creates a stopped test HTTP server and registers its endpoints. */
    server();

    /** Stops and destroys the test HTTP server. */
    ~server();

    server(const server&) = delete;
    server& operator=(const server&) = delete;
    server(server&&) = delete;
    server& operator=(server&&) = delete;

    /**
     * Starts the server on the IPv4 loopback interface using an available port.
     *
     * @throws std::logic_error    When the server is already running.
     * @throws std::runtime_error  When the server cannot bind to an available port.
     */
    void start();

    /** Stops the server and waits for its thread to finish. */
    void stop();

    /**
     * Returns the complete URL for an endpoint on this server.
     *
     * @param path  Absolute endpoint path.
     * @returns Complete endpoint URL.
     * @throws std::logic_error       When the server is not running.
     * @throws std::invalid_argument  When the endpoint path is empty or not absolute.
     */
    [[nodiscard]] std::string url(std::string_view path) const;

private:
    /** Underlying cpp-httplib server. */
    httplib::Server server_{};

    /** Thread running the blocking cpp-httplib listen loop. */
    std::jthread thread_{};

    /** Bound TCP port, or zero while the server is stopped. */
    int port_ = 0;
};

} // namespace kaycxx::http::test
