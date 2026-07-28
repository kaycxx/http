// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <sstream>
#include <string>
#include <string_view>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("sized_istream") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    describe("construction", [] {
        it("associates a stream with its known size", [] {
            client client{};
            const auto body = std::string{"Streamed request body"};
            auto input = std::istringstream{body};
            const auto response = client.post(test_server.url("/echo"), sized_istream{input, body.size()}, request_options{
                .headers = {
                    { "Content-Type", "text/plain" }
                }
            });

            assert_equal(response.text(), std::string_view{body});
        });

        it("limits reads to the declared size", [] {
            client client{};
            auto input = std::istringstream{"prefix and ignored suffix"};

            const auto response = client.post(test_server.url("/echo"), sized_istream{input, 6});

            assert_equal(response.text(), std::string_view{"prefix"});
        });
    });
}
