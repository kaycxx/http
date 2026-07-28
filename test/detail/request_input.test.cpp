// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <regex>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>

using namespace kaycxx::http;
using namespace kaycxx::test;

namespace {

/** Stream buffer throwing when data is read from it. */
class throwing_input_buffer final : public std::streambuf {
private:
    /** Throws instead of reading data. */
    std::streamsize xsgetn(char*, std::streamsize) override {
        throw std::logic_error{"Input stream failed"};
    }
};

} // namespace

suite("detail::request_input") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    it("propagates input stream exceptions", [] {
        client client{};
        auto buffer = throwing_input_buffer{};
        auto input = std::istream{&buffer};
        input.exceptions(std::ios::badbit);

        assert_throw<std::logic_error>(
            [&client, &input] { return client.post(test_server.url("/echo"), input); },
            std::regex{"Input stream failed"}
        );
    });

    describe("seek_body", [] {
        it("rewinds a streamed request body when following a redirect", [] {
            client client{};
            const auto body = std::string{"Redirected streamed body"};
            auto input = std::istringstream{body};
            const auto response = client.post(
                test_server.url("/redirect"),
                sized_istream{input, body.size()},
                request_options{
                    .follow_redirects = true
                }
            );

            assert_equal(response.text(), std::string_view{body});
        });

        it("rewinds an in-memory request body when following a temporary redirect", [] {
            client client{};
            const auto body = std::string{"Redirected in-memory GET body"};
            const auto response = client.get(
                test_server.url("/redirect-temporary"),
                body,
                request_options{
                    .follow_redirects = true
                }
            );

            assert_equal(response.text(), std::string_view{body});
        });

        it("retains a streamed GET body when following a temporary redirect", [] {
            client client{};
            const auto body = std::string{"Redirected streamed GET body"};
            auto input = std::istringstream{body};
            const auto response = client.get(
                test_server.url("/redirect-temporary"),
                sized_istream{input, body.size()},
                request_options{
                    .follow_redirects = true
                }
            );

            assert_equal(response.text(), std::string_view{body});
        });

        it("rewinds an in-memory POST body when following a method-preserving redirect", [] {
            client client{};
            const auto body = std::string{"Redirected in-memory POST body"};
            const auto response = client.post(test_server.url("/redirect"), body, request_options{
                .follow_redirects = true
            });

            assert_equal(response.text(), std::string_view{body});
        });
    });
}
