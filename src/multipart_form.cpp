// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/multipart_form.hpp>

#include <utility>

namespace kaycxx::http {

multipart_form::multipart_form() noexcept = default;

multipart_form::multipart_form(std::initializer_list<multipart_part> parts)
    : parts_{parts} {
}

void multipart_form::add(multipart_part part) {
    parts_.push_back(std::move(part));
}

} // namespace kaycxx::http
