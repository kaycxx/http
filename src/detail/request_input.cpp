// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include "request_input.hpp"

#include <kaycxx/http/error.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ios>
#include <istream>

namespace kaycxx::http::detail {

std::size_t read_body(char* data, std::size_t size, std::size_t count, void* user_data) noexcept {
    auto& input = *static_cast<request_input*>(user_data);
    const auto capacity = size * count;
    if (input.stream == nullptr) {
        const auto remaining = input.memory.size() - input.position;
        const auto bytes = std::min(capacity, remaining);
        if (bytes > 0) {
            std::memcpy(data, input.memory.data() + input.position, bytes);
            input.position += bytes;
        }
        return bytes;
    }

    auto& stream = *input.stream;
    if (stream.eof()) {
        return 0;
    }

    try {
        stream.read(data, static_cast<std::streamsize>(capacity));
        const auto bytes = stream.gcount();
        if (stream.bad() || (stream.fail() && !stream.eof())) {
            throw error{"Unable to read HTTP request body"};
        }
        return static_cast<std::size_t>(bytes);
    } catch (...) {
        if (stream.eof() && !stream.bad()) {
            return static_cast<std::size_t>(stream.gcount());
        }
        input.exception = std::current_exception();
        return CURL_READFUNC_ABORT;
    }
}

int seek_body(void* user_data, curl_off_t offset, int origin) noexcept {
    auto& input = *static_cast<request_input*>(user_data);
    if (input.stream == nullptr) {
        auto base = std::size_t{};
        switch (origin) {
            case SEEK_SET:
                break;
            case SEEK_CUR:
                base = input.position;
                break;
            case SEEK_END:
                base = input.memory.size();
                break;
            default:
                return CURL_SEEKFUNC_CANTSEEK;
        }

        if (offset < 0) {
            const auto distance = static_cast<std::uintmax_t>(-(offset + 1)) + 1;
            if (distance > base) {
                return CURL_SEEKFUNC_CANTSEEK;
            }
            input.position = base - static_cast<std::size_t>(distance);
        } else {
            const auto distance = static_cast<std::uintmax_t>(offset);
            if (distance > input.memory.size() - base) {
                return CURL_SEEKFUNC_CANTSEEK;
            }
            input.position = base + static_cast<std::size_t>(distance);
        }
        return CURL_SEEKFUNC_OK;
    }

    auto& stream = *input.stream;
    try {
        stream.clear();
        const auto stream_offset = static_cast<std::streamoff>(offset);
        switch (origin) {
            case SEEK_SET:
                if (input.start == std::streampos{-1}) {
                    return CURL_SEEKFUNC_CANTSEEK;
                }
                stream.seekg(input.start + stream_offset);
                break;
            case SEEK_CUR:
                stream.seekg(stream_offset, std::ios_base::cur);
                break;
            case SEEK_END:
                stream.seekg(stream_offset, std::ios_base::end);
                break;
            default:
                return CURL_SEEKFUNC_CANTSEEK;
        }
        return stream ? CURL_SEEKFUNC_OK : CURL_SEEKFUNC_CANTSEEK;
    } catch (...) {
        input.exception = std::current_exception();
        return CURL_SEEKFUNC_FAIL;
    }
}

std::streampos current_position(std::istream& stream) noexcept {
    try {
        return stream.rdbuf()->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
    } catch (...) {
        return std::streampos{-1};
    }
}

} // namespace kaycxx::http::detail
