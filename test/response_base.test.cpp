// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("response_base") {
    it("provides its status code", [] {
        const auto response = buffered_response{201, ""};

        assert_equal(response.status_code(), 201);
    });

    describe("headers", [] {
        it("returns all headers in their received order", [] {
            const auto response = buffered_response{200, "", {
                { "X-Test", "first" },
                { "x-test", "second" },
                { "X-Other", "other" }
            }};

            assert_equal(response.headers().size(), std::size_t{3});
            assert_equal(response.headers()[0].name, std::string{"X-Test"});
            assert_equal(response.headers()[0].value, std::string{"first"});
        });

        it("returns all matching values case-insensitively", [] {
            const auto response = buffered_response{200, "", {
                { "X-Test", "first" },
                { "x-test", "second" },
                { "X-Other", "other" }
            }};

            assert_equal(response.headers("X-Test"), std::vector<std::string>{"first", "second"});
            assert_true(response.headers("missing").empty());
        });

        it("moves all headers out of an expiring response", [] {
            const auto headers = buffered_response{200, "", {
                { "X-First", "first" },
                { "X-Second", "second" }
            }}.headers();

            static_assert(std::is_same_v<decltype(headers), const std::vector<header>>);
            assert_equal(headers.size(), std::size_t{2});
            assert_equal(headers[0].name, std::string{"X-First"});
            assert_equal(headers[0].value, std::string{"first"});
            assert_equal(headers[1].name, std::string{"X-Second"});
            assert_equal(headers[1].value, std::string{"second"});
        });
    });

    describe("header", [] {
        it("returns the first matching value case-insensitively", [] {
            const auto response = buffered_response{201, "", {
                { "X-Test", "first" },
                { "x-test", "second" },
                { "X-Other", "other" }
            }};

            assert_equal(response.header("x-TEST").value(), std::string_view{"first"});
            assert_false(response.header("missing").has_value());
        });

        it("moves the matching value out of an expiring response", [] {
            const auto value = buffered_response{200, "", {
                { "X-Test", "value" }
            }}.header("x-test");

            static_assert(std::is_same_v<decltype(value), const std::optional<std::string>>);
            assert_equal(value.value(), std::string{"value"});
        });
    });
}
