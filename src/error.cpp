// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/error.hpp>

#include <utility>

namespace kaycxx::http {

error::error(std::string message)
    : std::runtime_error{std::move(message)} {
}

error::~error() = default;

} // namespace kaycxx::http
