// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http/streamed_response.hpp>

#include <utility>

namespace kaycxx::http {

streamed_response::streamed_response(int status_code, std::vector<kaycxx::http::header> headers)
    : response_base{status_code, std::move(headers)} {
}

} // namespace kaycxx::http
