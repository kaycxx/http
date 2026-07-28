// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <string_view>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("error") {
    it("provides its message", [] {
        const error exception{"request failed"};

        assert_equal(std::string_view{exception.what()}, std::string_view{"request failed"});
    });
}
