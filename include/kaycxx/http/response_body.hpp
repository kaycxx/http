// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>
#include <iosfwd>

namespace kaycxx::http {

class client;

/** Non-owning HTTP response body destination. */
class response_body {
public:
    /**
     * Creates a response body destination.
     *
     * @param output  Output stream receiving the response body.
     */
    response_body(std::ostream& output) noexcept;

private:
    /** Output stream receiving the response body. */
    std::reference_wrapper<std::ostream> output_;

    friend class client;
};

} // namespace kaycxx::http
