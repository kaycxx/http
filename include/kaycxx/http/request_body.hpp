// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <kaycxx/http/sized_istream.hpp>

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace kaycxx::http {

class client;
class multipart_form;
class url_encoded_form;

/**
 * Non-owning HTTP request body descriptor.
 *
 * Referenced strings, byte containers, streams, JSON values and forms must remain valid until the synchronous request returns. An input stream is consumed from
 * its current position. When it is seekable, redirects and authentication retries can rewind it to that initial position.
 */
class request_body {
public:
    /** Creates an empty request body. */
    request_body() noexcept;

    /**
     * Creates a text request body.
     *
     * @param body  Text to send.
     */
    request_body(std::string_view body) noexcept;

    /**
     * Creates a text request body from a string.
     *
     * @param body  String to send.
     */
    request_body(const std::string& body) noexcept;

    /**
     * Creates a text request body from a null-terminated string.
     *
     * @param body  Null-terminated string to send.
     * @throws std::invalid_argument  When body is null.
     */
    request_body(const char* body);

    /**
     * Creates a binary request body.
     *
     * @param body  Bytes to send.
     */
    request_body(std::span<const std::byte> body) noexcept;

    /**
     * Creates a binary request body from a mutable byte span.
     *
     * @param body  Bytes to send.
     */
    request_body(std::span<std::byte> body) noexcept;

    /**
     * Creates a binary request body from a byte vector.
     *
     * @param body  Byte vector to send.
     */
    request_body(const std::vector<std::byte>& body) noexcept;

    /**
     * Creates a streamed request body with an unknown size.
     *
     * @param input  Input stream providing the request body.
     */
    request_body(std::istream& input) noexcept;

    /**
     * Creates a streamed request body with a known size.
     *
     * @param input  Sized input stream providing the request body.
     */
    request_body(sized_istream input) noexcept;

    /**
     * Creates a JSON request body.
     *
     * @param body  JSON value to serialize and send.
     */
    request_body(const nlohmann::json& body) noexcept;

    /**
     * Creates a URL-encoded form request body.
     *
     * @param body  URL-encoded form to serialize and send.
     */
    request_body(const url_encoded_form& body) noexcept;

    /**
     * Creates a multipart form request body.
     *
     * @param body  Multipart form to send.
     */
    request_body(const multipart_form& body) noexcept;

private:
    /** Input stream request body and its optional byte count. */
    struct stream_data {
        /** Input stream providing the request body. */
        std::reference_wrapper<std::istream> input;

        /** Optional number of bytes to read from the input stream. */
        std::optional<std::uint64_t> size;
    };

    /** Stored request body alternatives. */
    using value_type = std::variant<
        std::monostate,
        std::string_view,
        std::span<const std::byte>,
        stream_data,
        std::reference_wrapper<const nlohmann::json>,
        std::reference_wrapper<const url_encoded_form>,
        std::reference_wrapper<const multipart_form>
    >;

    /** Selected request body. */
    value_type value_{};

    friend class client;
};

} // namespace kaycxx::http
