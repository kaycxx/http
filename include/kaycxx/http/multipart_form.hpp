// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <kaycxx/http/multipart_part.hpp>

#include <initializer_list>
#include <vector>

namespace kaycxx::http {

class client;

/**
 * HTTP form encoded as `multipart/form-data`.
 *
 * A form must contain at least one part when it is sent.
 */
class multipart_form {
public:
    /** Creates an empty multipart form. */
    multipart_form() noexcept;

    /**
     * Creates a multipart form with the specified parts.
     *
     * Part order and duplicate field names are preserved.
     *
     * @param parts  Multipart form parts.
     */
    multipart_form(std::initializer_list<multipart_part> parts);

    /**
     * Adds a form part.
     *
     * @param part  Form part to add.
     */
    void add(multipart_part part);

private:
    /** Form parts in insertion order. */
    std::vector<multipart_part> parts_{};

    friend class client;
};

} // namespace kaycxx::http
