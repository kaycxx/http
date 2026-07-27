// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <kaycxx/http/header.hpp>
#include <kaycxx/http/request_body.hpp>

#include <optional>
#include <string>
#include <vector>

namespace kaycxx::http {

/** Part of a `multipart/form-data` request body. */
struct multipart_part {
    /** Form field name. */
    std::string name;

    /** Non-owning descriptor of the part body. */
    request_body body;

    /** Optional filename reported to the server. */
    std::optional<std::string> filename{};

    /** Optional content type overriding the default selected for the part body. */
    std::optional<std::string> content_type{};

    /** Additional part headers. `Content-Disposition` is managed by the client. */
    std::vector<header> headers{};
};

} // namespace kaycxx::http
