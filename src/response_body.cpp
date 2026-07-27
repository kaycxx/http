// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/response_body.hpp>

#include <ostream>

namespace kaycxx::http {

response_body::response_body(std::ostream& output) noexcept
    : output_{output} {
}

} // namespace kaycxx::http
