// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>

namespace kaycxx::http {

/** HTTP header consisting of a name and value. */
struct header {
    /** Header name. */
    std::string name;

    /** Header value. */
    std::string value;
};

} // namespace kaycxx::http
