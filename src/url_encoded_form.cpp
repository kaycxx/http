// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/url_encoded_form.hpp>

#include <string>
#include <string_view>

namespace kaycxx::http {

url_encoded_form::url_encoded_form() noexcept = default;

url_encoded_form::url_encoded_form(std::initializer_list<field> fields)
    : fields_{fields} {
}

void url_encoded_form::add(std::string_view name, std::string_view value) {
    fields_.emplace_back(std::string{name}, std::string{value});
}

} // namespace kaycxx::http
