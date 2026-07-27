// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kaycxx::http {

class client;

/** HTTP form encoded as `application/x-www-form-urlencoded`. */
class url_encoded_form {
public:
    /** Form field consisting of a name and value. */
    using field = std::pair<std::string, std::string>;

    /** Creates an empty URL-encoded form. */
    url_encoded_form() noexcept;

    /**
     * Creates a URL-encoded form with the specified fields.
     *
     * Field order and duplicate field names are preserved.
     *
     * @param fields  Form fields.
     */
    url_encoded_form(std::initializer_list<field> fields);

    /**
     * Adds a form field.
     *
     * @param name   Field name.
     * @param value  Field value.
     */
    void add(std::string_view name, std::string_view value);

private:
    /** Form fields in insertion order. */
    std::vector<field> fields_{};

    friend class client;
};

} // namespace kaycxx::http
