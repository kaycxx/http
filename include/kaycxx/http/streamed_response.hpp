// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <kaycxx/http/header.hpp>
#include <kaycxx/http/response_base.hpp>

#include <vector>

namespace kaycxx::http {

/** Successful HTTP response whose body has been written to an output stream. */
class streamed_response : public response_base {
public:
    /**
     * Creates a successful streamed HTTP response.
     *
     * @param status_code  HTTP status code in the range 200 to 299.
     * @param headers      Response headers.
     */
    explicit streamed_response(int status_code, std::vector<kaycxx::http::header> headers = {});
};

} // namespace kaycxx::http
