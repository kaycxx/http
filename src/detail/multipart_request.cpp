// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include "multipart_request.hpp"

#include <kaycxx/http/error.hpp>

#include <exception>
#include <memory>
#include <utility>

namespace kaycxx::http::detail {

multipart_request::multipart_request(CURL* handle)
    : mime_{curl_mime_init(handle), &curl_mime_free} {
    if (!mime_) {
        throw error{"Unable to create HTTP multipart form"};
    }
}

curl_mime* multipart_request::get() const noexcept {
    return mime_.get();
}

prepared_request_input& multipart_request::add_input() {
    auto input = std::make_unique<prepared_request_input>();
    auto& result = *input;
    inputs_.push_back(std::move(input));
    return result;
}

void multipart_request::rethrow_input_exception() const {
    for (const auto& input : inputs_) {
        if (input->input.exception) {
            std::rethrow_exception(input->input.exception);
        }
    }
}

} // namespace kaycxx::http::detail
