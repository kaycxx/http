// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

/**
 * @file
 * Declares internal HTTP syntax validation helpers.
 */

#include <string_view>

namespace kaycxx::http::detail {

/**
 * Validates an HTTP syntax token.
 *
 * @param value        Value to validate.
 * @param description  Human-readable value description for the error message.
 * @throws error  When the value is empty or contains a character not permitted in an HTTP token.
 */
void validate_http_token(std::string_view value, std::string_view description);

/**
 * Validates an HTTP header value.
 *
 * @param value        Value to validate.
 * @param description  Human-readable value description for the error message.
 * @throws error  When the value contains a control character not permitted in an HTTP header value.
 */
void validate_header_value(std::string_view value, std::string_view description);

} // namespace kaycxx::http::detail
