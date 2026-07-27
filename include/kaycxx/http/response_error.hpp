// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <kaycxx/http/buffered_response.hpp>
#include <kaycxx/http/error.hpp>

#include <string>

namespace kaycxx::http {

/**
 * Exception thrown when a server responds with a non-success status code.
 */
class response_error : public error {
public:
    /**
     * Creates an HTTP response error.
     *
     * @param message   Error message.
     * @param response  HTTP response returned by the server.
     */
    response_error(std::string message, buffered_response response);

    /** Destroys the HTTP response error. */
    ~response_error() override;

    /**
     * Returns the HTTP response which caused this error.
     *
     * The returned reference remains valid until this exception is destroyed, moved from or assigned a new value.
     *
     * @returns HTTP response returned by the server.
     */
    [[nodiscard]] const buffered_response& response() const & noexcept;

    /**
     * Moves the HTTP response out of an expiring response error.
     *
     * @returns HTTP response returned by the server.
     */
    [[nodiscard]] buffered_response response() && noexcept;

private:
    /** HTTP response returned by the server. */
    buffered_response response_;
};

} // namespace kaycxx::http
