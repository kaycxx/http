// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("client") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    it("performs a GET request", [] {
        client client{};
        const auto response = client.get(test_server.url("/get"));

        assert_equal(response.status_code(), 200);
        assert_equal(response.text(), std::string_view{"Hello from the test server"});
        const auto content_type = response.header("content-type");
        assert_true(content_type.has_value());
        assert_equal(*content_type, std::string_view{"text/plain"});
        const auto response_header = response.header("x-response-header");
        assert_true(response_header.has_value());
        assert_equal(*response_header, std::string_view{"response value"});
        assert_false(response.header("X-Missing-Header").has_value());

        auto multiple_values = response.headers("x-multiple-header");
        std::ranges::sort(multiple_values);
        assert_equal(multiple_values.size(), std::size_t{2});
        assert_equal(multiple_values[0], std::string{"first value"});
        assert_equal(multiple_values[1], std::string{"second value"});
    });

    it("performs a POST request without a body", [] {
        client client{};
        const auto response = client.post(test_server.url("/echo"));

        assert_true(response.text().empty());
    });

    it("performs DELETE requests with and without a body", [] {
        client client{};
        const auto body = std::string{"DELETE request body"};

        const auto response = client.del(test_server.url("/echo"), body);
        assert_equal(response.text(), std::string_view{body});

        const auto empty_response = client.del(test_server.url("/echo"));
        assert_true(empty_response.text().empty());
    });

    describe("move semantics", [] {
        it("rejects requests on a move-constructed source client", [] {
            client source{};
            client destination{std::move(source)};
            auto output = std::ostringstream{};

            assert_throw<error>(
                [&source] { return source.request("GET", "https://example.com"); },
                std::regex{".*moved from.*"}
            );
            assert_throw<error>(
                [&source] { return source.request("POST", "https://example.com", "body"); },
                std::regex{".*moved from.*"}
            );
            assert_throw<error>(
                [&source, &output] { return source.request("GET", "https://example.com", output); },
                std::regex{".*moved from.*"}
            );
            assert_throw<error>(
                [&source, &output] { return source.request("POST", "https://example.com", "body", output); },
                std::regex{".*moved from.*"}
            );

            assert_equal(destination.get(test_server.url("/get")).status_code(), 200);
        });

        it("rejects requests on a move-assigned source client", [] {
            client source{};
            client destination{};
            destination = std::move(source);

            assert_throw<error>(
                [&source] { return source.get("https://example.com"); },
                std::regex{".*moved from.*"}
            );
            assert_equal(destination.get(test_server.url("/get")).status_code(), 200);
        });
    });

    describe("request", [] {
        it("performs an arbitrary HEAD request", [] {
            client client{};
            const auto response = client.request("HEAD", test_server.url("/get"));

            assert_equal(response.status_code(), 200);
            assert_equal(response.text(), std::string_view{});
            assert_equal(response.header("X-Response-Header").value(), std::string_view{"response value"});
        });

        it("performs a valid custom request method", [] {
            client client{};
            const auto response = client.request("OPTIONS", test_server.url("/method"));

            assert_equal(response.text(), std::string_view{"OPTIONS"});
        });

        it("streams an arbitrary request method", [] {
            client client{};
            auto output = std::ostringstream{};

            const auto response = client.request("OPTIONS", test_server.url("/method"), response_body{output});

            assert_equal(response.status_code(), 200);
            assert_equal(output.str(), std::string{"OPTIONS"});
        });
    });

}
