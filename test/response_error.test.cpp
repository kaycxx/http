// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <string>
#include <string_view>
#include <type_traits>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("response_error") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    describe("response", [] {
        it("exposes the response returned by the server", [] {
            client client{};
            const auto url = test_server.url("/error");

            try {
                const auto response = client.get(url);
                assert_true(
                    false,
                    std::string{"Expected the request to throw a response error but received HTTP status "}
                        + std::to_string(response.status_code())
                );
            } catch (const response_error& exception) {
                const auto& response = exception.response();
                assert_equal(std::string_view{exception.what()}, std::string_view{"GET request failed: HTTP status 400"});
                assert_equal(response.status_code(), 400);
                const auto error_header = response.header("x-error-header");
                assert_true(error_header.has_value());
                assert_equal(*error_header, std::string_view{"error value"});
                assert_equal(response.text(), std::string_view{R"({"error":"Invalid request"})"});
                assert_equal(response.json().at("error").get<std::string>(), std::string{"Invalid request"});
            }
        });

        it("returns the stored response", [] {
            const response_error exception{"not found", buffered_response{404, R"({"error":"Not found"})"}};
            const auto& response = exception.response();

            assert_equal(std::string_view{exception.what()}, std::string_view{"not found"});
            assert_equal(response.status_code(), 404);
            assert_equal(response.text(), std::string_view{R"({"error":"Not found"})"});
            assert_equal(response.json().at("error").get<std::string>(), std::string{"Not found"});
        });

        it("moves the response out of an expiring error", [] {
            const auto response = response_error{
                "not found",
                buffered_response{404, "response body"}
            }.response();

            static_assert(std::is_same_v<decltype(response), const buffered_response>);
            assert_equal(response.status_code(), 404);
            assert_equal(response.text(), std::string_view{"response body"});
        });
    });
}
