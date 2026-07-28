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

/** Converts JSON integer byte values to a string. */
[[nodiscard]] std::string byte_text(const nlohmann::json& value) {
    auto result = std::string{};
    result.reserve(value.size());
    for (const auto& byte : value) {
        result.push_back(static_cast<char>(byte.get<unsigned char>()));
    }
    return result;
}

} // namespace

suite("detail::multipart_request") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    it("rewinds a streamed multipart part when following a redirect", [] {
        client client{};
        const auto body = std::string{"redirected multipart stream"};
        auto input = std::istringstream{body};
        const auto response = client.post(
            test_server.url("/redirect-multipart"),
            multipart_form{
                {
                    .name = "stream",
                    .body = input,
                    .filename = "stream.txt",
                    .content_type = "text/plain"
                }
            },
            request_options{
                .follow_redirects = true
            }
        ).json();

        assert_equal(
            byte_text(response.at("parts").at("stream").at(0).at("bytes")),
            body
        );
    });

    it("propagates multipart input stream exceptions", [] {
        client client{};
        auto buffer = throwing_input_buffer{};
        auto input = std::istream{&buffer};
        input.exceptions(std::ios::badbit);
        const auto form = multipart_form{
            {
                .name = "stream",
                .body = input
            }
        };

        assert_throw<std::logic_error>(
            [&client, &form] { return client.post(test_server.url("/multipart-form"), form); },
            std::regex{"Input stream failed"}
        );
    });
}
