// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <concepts>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>

using namespace kaycxx::http;
using namespace kaycxx::test;

namespace {

/** Stream buffer throwing when data is written to it. */
class throwing_output_buffer final : public std::streambuf {
private:
    /** Throws instead of writing data. */
    std::streamsize xsputn(const char*, std::streamsize) override {
        throw std::logic_error{"Output stream failed"};
    }
};

/** Stream which can be used for both input and output. */
class bidirectional_stream : public std::iostream {
public:
    using std::iostream::iostream;
};

/** Whether a stream is unambiguously accepted as a streamed response body. */
template<typename stream_type>
concept response_body_stream = requires(client& http, std::string_view url, stream_type& stream) {
    { http.post(url, stream) } -> std::same_as<streamed_response>;
};

static_assert(!response_body_stream<std::ifstream>);
static_assert(!response_body_stream<std::istringstream>);
static_assert(response_body_stream<std::ofstream>);
static_assert(response_body_stream<std::ostringstream>);
static_assert(!response_body_stream<std::fstream>);
static_assert(!response_body_stream<std::stringstream>);
static_assert(!response_body_stream<bidirectional_stream>);

} // namespace

suite("response_body") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    it("streams only the final response body and headers after a redirect", [] {
        client client{};
        auto output = std::ostringstream{};

        const auto response = client.get(test_server.url("/redirect-with-body"), output, request_options{
            .follow_redirects = true
        });

        assert_equal(output.str(), std::string{"Hello from the test server"});
        assert_false(response.header("X-Intermediate-Header").has_value());
        assert_equal(response.header("X-Response-Header").value(), std::string_view{"response value"});
    });

    it("propagates output stream exceptions", [] {
        client client{};
        auto buffer = throwing_output_buffer{};
        auto output = std::ostream{&buffer};
        output.exceptions(std::ios::badbit);

        assert_throw<std::logic_error>(
            [&client, &output] { return client.get(test_server.url("/get"), output); },
            std::regex{"Output stream failed"}
        );
    });

    it("streams request and response bodies together", [] {
        client client{};
        const auto body = std::string{"Bidirectional streamed body"};
        auto input = std::istringstream{body};
        auto output = std::ostringstream{};
        const auto response = client.put(test_server.url("/echo"), input, output, request_options{
            .headers = {
                { "Content-Type", "application/octet-stream" }
            }
        });

        assert_equal(response.status_code(), 200);
        assert_equal(output.str(), body);
    });

    it("streams a JSON response body", [] {
        client client{};
        auto output = std::ostringstream{};
        const auto response = client.post(test_server.url("/echo"), nlohmann::json{
            { "message", "streamed JSON response" }
        }, output);

        assert_equal(response.status_code(), 200);
        assert_equal(nlohmann::json::parse(output.str()).at("message").get<std::string>(), std::string{"streamed JSON response"});
    });

    it("keeps a streamed error response out of the output stream", [] {
        client client{};
        auto output = std::ostringstream{};

        try {
            const auto response = client.get(test_server.url("/error"), output);
            assert_true(
                false,
                std::string{"Expected the request to throw a response error but received HTTP status "}
                    + std::to_string(response.status_code())
            );
        } catch (const response_error& exception) {
            const auto& response = exception.response();
            assert_equal(output.str(), std::string{});
            assert_equal(response.status_code(), 400);
            assert_equal(response.text(), std::string_view{R"({"error":"Invalid request"})"});
        }
    });

    describe("output selection", [] {
        it("streams a successful response body", [] {
            client client{};
            auto output = std::ostringstream{};

            const auto response = client.get(test_server.url("/get"), output);

            assert_equal(response.status_code(), 200);
            const auto response_header = response.header("X-Response-Header");
            assert_true(response_header.has_value());
            assert_equal(*response_header, std::string_view{"response value"});
            assert_equal(output.str(), std::string{"Hello from the test server"});
        });

        it("accepts an explicitly selected bidirectional stream", [] {
            client client{};
            auto stream = std::stringstream{};

            const auto response = client.get(test_server.url("/get"), response_body{stream});

            assert_equal(response.status_code(), 200);
            assert_equal(stream.str(), std::string{"Hello from the test server"});
        });
    });
}
