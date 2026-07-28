// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <string_view>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("streamed_response") {
    it("provides its status code and headers", [] {
        const streamed_response response{206, {
            { "Content-Range", "bytes 0-2/10" }
        }};

        assert_equal(response.status_code(), 206);
        assert_equal(response.header("content-range").value(), std::string_view{"bytes 0-2/10"});
    });
}
