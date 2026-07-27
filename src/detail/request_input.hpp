// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

/**
 * @file
 * Declares internal request body input state and callbacks.
 */

#include <curl/curl.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <ios>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace kaycxx::http::detail {

/** Request body source used by the libcurl read and seek callbacks. */
struct request_input {
    /** In-memory request body. */
    std::span<const char> memory{};

    /** Current read position in the in-memory request body. */
    std::size_t position = 0;

    /** Input stream providing the request body. */
    std::istream* stream = nullptr;

    /** Stream position at which the request body starts, or an invalid position when the stream is not seekable. */
    std::streampos start = std::streampos{-1};

    /** Exception thrown while reading the request body. */
    std::exception_ptr exception{};
};

/** Prepared request body input and metadata. */
struct prepared_request_input {
    /** Owned serialized request body data. */
    std::string storage{};

    /** Request body source used by libcurl callbacks. */
    request_input input{};

    /** Optional request body byte count. */
    std::optional<std::uintmax_t> size{};

    /** Default content type selected for this body kind. */
    std::string_view default_content_type{};

    /** Whether a request body is present. */
    bool has_body = false;
};

/**
 * Reads request body data from memory or an input stream.
 *
 * @param data       Target buffer.
 * @param size       Size of each requested element.
 * @param count      Number of requested elements.
 * @param user_data  Request input state.
 * @returns Number of provided bytes, zero at end of input or `CURL_READFUNC_ABORT` when reading failed.
 */
std::size_t read_body(char* data, std::size_t size, std::size_t count, void* user_data) noexcept;

/**
 * Repositions a request body source when libcurl needs to resend its body.
 *
 * @param user_data  Request input state.
 * @param offset     Requested byte offset.
 * @param origin     Offset origin using the standard `SEEK_SET`, `SEEK_CUR` and `SEEK_END` values.
 * @returns `CURL_SEEKFUNC_OK` on success, `CURL_SEEKFUNC_CANTSEEK` when the requested position cannot be reached or
 *          `CURL_SEEKFUNC_FAIL` when seeking threw an exception.
 */
int seek_body(void* user_data, curl_off_t offset, int origin) noexcept;

/**
 * Determines the current position of a request input stream without changing its state.
 *
 * @param stream  Input stream to inspect.
 * @returns Current stream position, or an invalid position when the stream is not seekable.
 */
[[nodiscard]] std::streampos current_position(std::istream& stream) noexcept;

} // namespace kaycxx::http::detail
