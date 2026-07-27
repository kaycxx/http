// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <stdexcept>
#include <string>

namespace kaycxx::http {

/**
 * Base exception for HTTP client errors.
 */
class error : public std::runtime_error {
public:
    /**
     * Creates an HTTP client error.
     *
     * @param message  Error message.
     */
    explicit error(std::string message);

    /** Destroys the HTTP client error. */
    ~error() override;
};

} // namespace kaycxx::http
