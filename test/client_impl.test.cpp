// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <chrono>
#include <cstddef>
#include <limits>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("client_impl") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    it("leaves request overrides unset", [] {
        const request_options options{};

        assert_equal(options.follow_redirects, std::optional<bool>{});
        assert_equal(options.max_redirects, std::optional<std::size_t>{});
        assert_equal(options.connect_timeout, std::optional<std::chrono::milliseconds>{});
        assert_equal(options.request_timeout, std::optional<std::chrono::milliseconds>{});
        assert_false(static_cast<bool>(options.progress));
        assert_equal(options.max_buffered_response_size, std::optional<std::size_t>{});
    });

    describe("perform", [] {
        it("inherits and overrides client request headers", [] {
            client client{request_options{
                .headers = {
                    { "X-Test-Header", "client value" }
                }
            }};

            const auto inherited = client.get(test_server.url("/header"));
            assert_equal(inherited.text(), std::string_view{"client value"});

            const auto retained = client.get(test_server.url("/header"), request_options{
                .headers = {
                    { "X-Unrelated-Header", "request value" }
                }
            });
            assert_equal(retained.text(), std::string_view{"client value"});

            const auto overridden = client.get(test_server.url("/header"), request_options{
                .headers = {
                    { "x-test-header", "request value" }
                }
            });
            assert_equal(overridden.text(), std::string_view{"request value"});

            const auto inherited_again = client.get(test_server.url("/header"));
            assert_equal(inherited_again.text(), std::string_view{"client value"});
        });

        it("does not follow redirects by default", [] {
            client client{};

            try {
                const auto response = client.get(test_server.url("/redirect-with-body"));
                assert_true(
                    false,
                    std::string{"Expected the request to throw a response error but received HTTP status "}
                        + std::to_string(response.status_code())
                );
            } catch (const response_error& exception) {
                assert_equal(exception.response().status_code(), 302);
                assert_equal(exception.response().text(), std::string_view{"Intermediate redirect body"});
            }
        });

        it("does not follow redirects when explicitly disabled", [] {
            client client{};

            try {
                const auto response = client.get(test_server.url("/redirect-with-body"), request_options{
                    .follow_redirects = false
                });
                assert_true(
                    false,
                    std::string{"Expected the request to throw a response error but received HTTP status "}
                        + std::to_string(response.status_code())
                );
            } catch (const response_error& exception) {
                assert_equal(exception.response().status_code(), 302);
                assert_equal(exception.response().text(), std::string_view{"Intermediate redirect body"});
            }
        });

        it("inherits and overrides client redirect handling", [] {
            client client{request_options{
                .follow_redirects = true
            }};

            const auto inherited = client.get(test_server.url("/redirect-with-body"));
            assert_equal(inherited.status_code(), 200);

            assert_throw<response_error>(
                [&client] {
                    return client.get(test_server.url("/redirect-with-body"), request_options{
                        .follow_redirects = false
                    });
                },
                std::regex{".*HTTP status 302.*"}
            );

            const auto inherited_again = client.get(test_server.url("/redirect-with-body"));
            assert_equal(inherited_again.status_code(), 200);
        });

        it("applies the configured redirect limit", [] {
            client client{};

            assert_throw<error>(
                [&client] {
                    return client.get(test_server.url("/redirect-with-body"), request_options{
                        .follow_redirects = true,
                        .max_redirects = 0
                    });
                },
                std::regex{".*redirect.*"}
            );

            const auto response = client.get(test_server.url("/redirect-with-body"), request_options{
                .follow_redirects = true
            });
            assert_equal(response.status_code(), 200);
        });

        it("applies the configured request timeout without retaining it", [] {
            client client{};

            assert_throw<error>(
                [&client] {
                    return client.get(test_server.url("/slow"), request_options{
                        .request_timeout = std::chrono::milliseconds{10}
                    });
                },
                std::regex{".*([Tt]imeout|timed out).*"}
            );

            const auto response = client.get(test_server.url("/slow"));
            assert_equal(response.text(), std::string_view{"Slow response"});
        });

        it("inherits and overrides the client request timeout", [] {
            client client{request_options{
                .request_timeout = std::chrono::milliseconds{10}
            }};

            assert_throw<error>(
                [&client] { return client.get(test_server.url("/slow")); },
                std::regex{".*([Tt]imeout|timed out).*"}
            );

            const auto overridden = client.get(test_server.url("/slow"), request_options{
                .request_timeout = std::chrono::seconds{1}
            });
            assert_equal(overridden.text(), std::string_view{"Slow response"});

            assert_throw<error>(
                [&client] { return client.get(test_server.url("/slow")); },
                std::regex{".*([Tt]imeout|timed out).*"}
            );
        });

        it("inherits and overrides the client progress callback", [] {
            auto client_updates = std::size_t{};
            client client{request_options{
                .progress = [&client_updates](const auto&) {
                    ++client_updates;
                }
            }};

            [[maybe_unused]] const auto inherited = client.get(test_server.url("/get"));
            assert_true(client_updates > 0);

            const auto previous_client_updates = client_updates;
            auto request_updates = std::size_t{};
            [[maybe_unused]] const auto overridden = client.get(test_server.url("/get"), request_options{
                .progress = [&request_updates](const auto&) {
                    ++request_updates;
                }
            });

            assert_true(request_updates > 0);
            assert_equal(client_updates, previous_client_updates);
        });

        it("rejects a reentrant request on the same client", [] {
            client client{};

            assert_throw<error>(
                [&client] {
                    return client.get(test_server.url("/get"), request_options{
                        .progress = [&client](const auto&) {
                            [[maybe_unused]] const auto response = client.get(test_server.url("/get"));
                        }
                    });
                },
                std::regex{".*request already in progress.*"}
            );

            const auto response = client.get(test_server.url("/get"));
            assert_equal(response.status_code(), 200);
        });

        it("rejects a redirect limit which cannot be represented by libcurl", [] {
            client client{};

            assert_throw<error>(
                [&client] {
                    return client.get(test_server.url("/get"), request_options{
                        .follow_redirects = true,
                        .max_redirects = std::numeric_limits<std::size_t>::max()
                    });
                },
                std::regex{".*redirect limit.*supported range.*"}
            );
        });

        it("inherits and overrides the client buffered response body limit", [] {
            client client{request_options{
                .max_buffered_response_size = 4
            }};

            assert_throw<error>(
                [&client] { return client.get(test_server.url("/get")); },
                std::regex{".*buffer limit.*"}
            );

            const auto unlimited = client.get(test_server.url("/get"), request_options{
                .max_buffered_response_size = 0
            });
            assert_equal(unlimited.text(), std::string_view{"Hello from the test server"});

            assert_throw<error>(
                [&client] { return client.get(test_server.url("/get")); },
                std::regex{".*buffer limit.*"}
            );
        });

        it("obeys redirect status codes for custom request methods", [] {
            client client{};
            const auto response = client.del(
                test_server.url("/redirect-see-other"),
                "Request body",
                request_options{
                    .follow_redirects = true
                }
            );

            assert_equal(response.text(), std::string_view{"GET"});
        });

        it("changes a bodyless PUT to GET after a see-other redirect", [] {
            client client{};
            const auto response = client.put(test_server.url("/redirect-see-other"), request_options{
                .follow_redirects = true
            });

            assert_equal(response.text(), std::string_view{"GET"});
        });

        it("rejects non-HTTP URL schemes", [] {
            client client{};
            constexpr auto url = std::string_view{"file:///secret/request-target"};

            try {
                const auto response = client.get(url);
                assert_true(
                    false,
                    std::string{"Expected the request to throw an error but received HTTP status "}
                        + std::to_string(response.status_code())
                );
            } catch (const error& exception) {
                const auto message = std::string_view{exception.what()};
                assert_true(message.starts_with("GET request failed: "));
                assert_true(message.contains("file"));
                assert_true(message.contains("disabled"));
                assert_false(message.contains(url));
            }
        });
    });

}
