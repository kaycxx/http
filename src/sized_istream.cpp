// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/sized_istream.hpp>

#include <functional>

namespace kaycxx::http {

sized_istream::sized_istream(std::istream& input, std::uint64_t size) noexcept
    : input_{std::ref(input)}, size_{size} {
}

} // namespace kaycxx::http
