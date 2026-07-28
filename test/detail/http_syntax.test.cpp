// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <regex>
#include <string>
#include <string_view>
#include <vector>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("detail::http_syntax") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    describe("validate_http_token", [] {
        it("rejects invalid request methods", [] {
            client client{};
            const auto invalid_methods = std::vector<std::string_view>{
                "",
                "GET POST",
                "GET\tPOST",
                "GET\rPOST",
                "GET\nPOST",
                "GET\vPOST",
                "GET/POST",
                "GET:POST",
                "GET(POST)",
                "GÜT",
                std::string_view{"GET\0POST", 8}
            };

            for (const auto method : invalid_methods) {
                assert_throw<error>(
                    [&client, method] { return client.request(method, test_server.url("/get")); },
                    std::regex{".*Invalid HTTP request method.*"}
                );
            }

            const auto response = client.get(test_server.url("/get"));
            assert_equal(response.status_code(), 200);
        });

        it("rejects invalid request header names", [] {
            client client{};
            const auto invalid_names = std::vector<std::string>{
                "",
                "X Test",
                "X\tTest",
                "X:Test",
                "X\rTest",
                "X\nTest",
                "X(Test)",
                "X/Test",
                "Grüße",
                std::string{"X\0Test", 6}
            };

            for (const auto& name : invalid_names) {
                assert_throw<error>(
                    [&client, &name] {
                        return client.get(test_server.url("/get"), request_options{
                            .headers = {
                                { name, "value" }
                            }
                        });
                    },
                    std::regex{".*Invalid HTTP request header name.*"}
                );
            }
        });

        it("rejects invalid multipart part header names", [] {
            client client{};
            const auto form = multipart_form{
                {
                    .name = "value",
                    .body = "body",
                    .headers = {
                        { "X Part", "value" }
                    }
                }
            };

            assert_throw<error>(
                [&client, &form] { return client.post(test_server.url("/multipart-form"), form); },
                std::regex{".*HTTP multipart part header name.*"}
            );
        });
    });

    describe("validate_header_value", [] {
        it("accepts structurally valid request header values", [] {
            client client{};
            const auto value = std::string{"Grüße\tvalue: still data"};
            const auto response = client.get(test_server.url("/header"), request_options{
                .headers = {
                    { "X-Test-Header", value }
                }
            });

            assert_equal(response.text(), std::string_view{value});
        });

        it("rejects invalid request header values", [] {
            client client{};
            const auto invalid_values = std::vector<std::string>{
                "value\rX-Injected: true",
                "value\nX-Injected: true",
                std::string{"value"} + '\0' + "suffix",
                std::string{"value"} + '\v',
                std::string{"value"} + '\f',
                std::string{"value"} + static_cast<char>(0x1f),
                std::string{"value"} + static_cast<char>(0x7f)
            };

            for (const auto& value : invalid_values) {
                assert_throw<error>(
                    [&client, &value] {
                        return client.get(test_server.url("/get"), request_options{
                            .headers = {
                                { "X-Test-Header", value }
                            }
                        });
                    },
                    std::regex{".*HTTP request header value.*invalid control character.*"}
                );
            }
        });

        it("rejects invalid multipart content types", [] {
            client client{};
            const auto invalid_values = std::vector<std::string>{
                "text/plain\rX-Injected: true",
                "text/plain\nX-Injected: true",
                std::string{"text/plain"} + '\0' + "suffix",
                std::string{"text/plain"} + '\v',
                std::string{"text/plain"} + '\f',
                std::string{"text/plain"} + static_cast<char>(0x1f),
                std::string{"text/plain"} + static_cast<char>(0x7f)
            };

            for (const auto& value : invalid_values) {
                const auto form = multipart_form{
                    {
                        .name = "value",
                        .body = "body",
                        .content_type = value
                    }
                };

                assert_throw<error>(
                    [&client, &form] { return client.post(test_server.url("/multipart-form"), form); },
                    std::regex{".*multipart part content type.*invalid control character.*"}
                );
            }
        });

        it("rejects invalid multipart part header values", [] {
            client client{};
            const auto form = multipart_form{
                {
                    .name = "value",
                    .body = "body",
                    .headers = {
                        { "X-Part", "value\r\nX-Injected: true" }
                    }
                }
            };

            assert_throw<error>(
                [&client, &form] { return client.post(test_server.url("/multipart-form"), form); },
                std::regex{".*HTTP multipart part header value.*"}
            );
        });
    });
}
