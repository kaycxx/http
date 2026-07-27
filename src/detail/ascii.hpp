// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <curl/curl.h>

#include <string_view>

namespace kaycxx::http::detail {

/**
 * Compares two ASCII strings without regard to letter case.
 *
 * @param first   First string.
 * @param second  Second string.
 * @returns True when both strings are equal ignoring ASCII letter case.
 */
[[nodiscard]] inline bool equals_ignore_case(std::string_view first, std::string_view second) noexcept {
    if (first.size() != second.size()) {
        return false;
    }
    return first.empty() || curl_strnequal(first.data(), second.data(), first.size()) != 0;
}

} // namespace kaycxx::http::detail
