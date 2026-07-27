// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include "transfer_state.hpp"

#include <kaycxx/http/error.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <ostream>
#include <string>

namespace kaycxx::http::detail {

std::size_t write_body(char* data, std::size_t size, std::size_t count, void* user_data) noexcept {
    auto& output = *static_cast<response_state*>(user_data);
    const auto bytes = size * count;
    try {
        // Query and cache the status code when the first body chunk of a streamed response is received.
        if (output.stream != nullptr && output.status_code == 0) {
            auto status_code = 0L;
            output.status_result = curl_easy_getinfo(output.handle, CURLINFO_RESPONSE_CODE, &status_code);
            if (output.status_result != CURLE_OK) {
                return CURL_WRITEFUNC_ERROR;
            }
            output.status_code = static_cast<int>(status_code);
        }

        // Process every body chunk, streaming successful responses and buffering all other responses.
        if (output.stream != nullptr && output.status_code >= 200 && output.status_code < 300) {
            output.stream->write(data, static_cast<std::streamsize>(bytes));
            if (!*output.stream) {
                throw error{"Unable to write HTTP response body"};
            }
        } else {
            if (output.max_buffered_response_size.has_value()
                    && (output.body.size() > *output.max_buffered_response_size
                        || bytes > *output.max_buffered_response_size - output.body.size())) {
                throw error{
                    "HTTP response body exceeds configured buffer limit of "
                        + std::to_string(*output.max_buffered_response_size) + " bytes"
                };
            }
            output.body.append(data, bytes);
        }
    } catch (...) {
        output.exception = std::current_exception();
        return CURL_WRITEFUNC_ERROR;
    }
    return bytes;
}

int report_progress(
    void* user_data,
    curl_off_t download_total,
    curl_off_t downloaded,
    curl_off_t upload_total,
    curl_off_t uploaded
) noexcept {
    auto& state = *static_cast<progress_state*>(user_data);
    try {
        state.callback(transfer_progress{
            .downloaded = static_cast<std::uint64_t>(downloaded),
            .download_total = static_cast<std::uint64_t>(download_total),
            .uploaded = static_cast<std::uint64_t>(uploaded),
            .upload_total = static_cast<std::uint64_t>(upload_total)
        });
    } catch (...) {
        state.exception = std::current_exception();
        return 1;
    }
    return 0;
}

} // namespace kaycxx::http::detail
