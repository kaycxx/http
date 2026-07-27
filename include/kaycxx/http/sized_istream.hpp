// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>

namespace kaycxx::http {

class request_body;

/** Input stream request body with a known byte count. */
class sized_istream {
public:
    /**
     * Associates an input stream with the number of bytes to read from it.
     *
     * @param input  Input stream providing the request body.
     * @param size   Number of bytes to read from the input stream.
     */
    sized_istream(std::istream& input, std::uint64_t size) noexcept;

private:
    /** Input stream providing the request body. */
    std::reference_wrapper<std::istream> input_;

    /** Number of bytes to read from the input stream. */
    std::uint64_t size_;

    friend class request_body;
};

} // namespace kaycxx::http
