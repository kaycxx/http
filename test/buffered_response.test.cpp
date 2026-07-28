// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <regex>
#include <string>
#include <string_view>
#include <type_traits>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("buffered_response") {
    describe("text", [] {
        it("returns the response body", [] {
            const buffered_response response{200, "response body"};

            assert_equal(response.text(), std::string_view{"response body"});
        });

        it("moves the body out of an expiring response", [] {
            const auto text = buffered_response{200, "response body"}.text();

            static_assert(std::is_same_v<decltype(text), const std::string>);
            assert_equal(text, std::string{"response body"});
        });
    });

    describe("json", [] {
        it("parses the response body", [] {
            const buffered_response response{200, R"({"success":true})"};

            assert_true(response.json().at("success").get<bool>());
        });

        it("represents an empty body as null", [] {
            const buffered_response response{204, ""};

            assert_true(response.json().is_null());
        });

        it("propagates parse errors", [] {
            const buffered_response response{200, "{"};

            assert_throw<nlohmann::json::parse_error>(
                [&response] { return response.json(); },
                std::regex{".*json\\.exception\\.parse_error.*"}
            );
        });
    });
}
