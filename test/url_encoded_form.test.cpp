// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <string>
#include <string_view>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("url_encoded_form") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    describe("serialization", [] {
        it("preserves and encodes form fields", [] {
            client client{};
            auto form = url_encoded_form{
                { "name", "Ada Lovelace" },
                { "duplicate", "first value" },
                { "duplicate", "second+value" },
                { "special name +&=", "space + plus & ampersand = equals % percent / slash ?" },
                { "symbols", "*-._~" },
                { "empty", "" }
            };
            form.add("unicode", "Grüße");

            const auto raw_response = client.post(test_server.url("/echo"), form);
            assert_equal(
                raw_response.text(),
                std::string_view{
                    "name=Ada+Lovelace&duplicate=first+value&duplicate=second%2Bvalue&"
                    "special+name+%2B%26%3D=space+%2B+plus+%26+ampersand+%3D+equals+%25+percent+%2F+slash+%3F&"
                    "symbols=*-._%7E&empty=&unicode=Gr%C3%BC%C3%9Fe"
                }
            );
            assert_equal(
                raw_response.header("Content-Type").value(),
                std::string_view{"application/x-www-form-urlencoded"}
            );

            const auto parsed_response = client.post(test_server.url("/url-encoded-form"), form).json();
            const auto& fields = parsed_response.at("fields");
            assert_equal(
                parsed_response.at("content_type").get<std::string>(),
                std::string{"application/x-www-form-urlencoded"}
            );
            assert_equal(fields.at("name").at(0).get<std::string>(), std::string{"Ada Lovelace"});
            assert_equal(fields.at("duplicate").at(0).get<std::string>(), std::string{"first value"});
            assert_equal(fields.at("duplicate").at(1).get<std::string>(), std::string{"second+value"});
            assert_equal(
                fields.at("special name +&=").at(0).get<std::string>(),
                std::string{"space + plus & ampersand = equals % percent / slash ?"}
            );
            assert_equal(fields.at("symbols").at(0).get<std::string>(), std::string{"*-._~"});
            assert_equal(fields.at("empty").at(0).get<std::string>(), std::string{});
            assert_equal(fields.at("unicode").at(0).get<std::string>(), std::string{"Grüße"});
        });

        it("keeps an empty form as a present request body", [] {
            client client{};
            const auto response = client.post(test_server.url("/echo"), url_encoded_form{});

            assert_true(response.text().empty());
            assert_equal(
                response.header("Content-Type").value(),
                std::string_view{"application/x-www-form-urlencoded"}
            );
        });
    });
}
