// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace kaycxx::http {

/** Current HTTP transfer progress reported by libcurl. */
struct transfer_progress {
    /** Number of bytes downloaded so far. */
    std::uint64_t downloaded{};

    /** Expected total number of downloaded bytes, or zero when unknown, unused or empty. */
    std::uint64_t download_total{};

    /** Number of bytes uploaded so far. */
    std::uint64_t uploaded{};

    /** Expected total number of uploaded bytes, or zero when unknown, unused or empty. */
    std::uint64_t upload_total{};
};

} // namespace kaycxx::http
