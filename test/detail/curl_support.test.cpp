// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <regex>
#include <string>
#include <string_view>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("detail::curl_support") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    describe("append_headers", [] {
        it("sends configured request headers", [] {
            client client{};
            const auto response = client.get(test_server.url("/header"), request_options{
                .headers = {
                    { "X-Test-Header", "configured value" }
                }
            });

            assert_equal(response.text(), std::string_view{"configured value"});

            const auto response_without_header = client.get(test_server.url("/header"));
            assert_true(response_without_header.text().empty());
        });

        it("sends an explicitly empty request header", [] {
            client client{};
            const auto response = client.get(test_server.url("/empty-header"), request_options{
                .headers = {
                    { "X-Empty-Header", "" }
                }
            });

            assert_equal(response.text(), std::string_view{"present"});
        });

        it("rejects request framing headers", [] {
            client client{};
            for (const auto name : { "Content-Length", "transfer-encoding" }) {
                assert_throw<error>(
                    [&client, name] {
                        return client.post(test_server.url("/echo"), "body", request_options{
                            .headers = {
                                { name, "4" }
                            }
                        });
                    },
                    std::regex{".*HTTP request .* header.*managed by the client.*"}
                );
            }
        });

        it("rejects a custom multipart request content type", [] {
            client client{};
            const auto form = multipart_form{
                {
                    .name = "value",
                    .body = "body"
                }
            };

            assert_throw<error>(
                [&client, &form] {
                    return client.post(test_server.url("/multipart-form"), form, request_options{
                        .headers = {
                            { "content-type", "multipart/form-data; boundary=custom" }
                        }
                    });
                },
                std::regex{".*multipart request Content-Type header.*managed by the client.*"}
            );
        });

        it("rejects a custom multipart content disposition", [] {
            client client{};
            const auto form = multipart_form{
                {
                    .name = "value",
                    .body = "body",
                    .headers = {
                        { "content-disposition", "form-data; name=override" }
                    }
                }
            };

            assert_throw<error>(
                [&client, &form] { return client.post(test_server.url("/multipart-form"), form); },
                std::regex{".*multipart part Content-Disposition header.*managed by the client.*"}
            );
        });
    });
}
