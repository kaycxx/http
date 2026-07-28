// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <fstream>
#include <regex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace kaycxx::http;
using namespace kaycxx::test;

namespace {

/** Stream which can be used for both input and output. */
class bidirectional_stream : public std::iostream {
public:
    using std::iostream::iostream;
};

/** Whether a stream is unambiguously accepted as a buffered request body. */
template<typename stream_type>
concept request_body_stream = requires(client& http, std::string_view url, stream_type& stream) {
    { http.post(url, stream) } -> std::same_as<buffered_response>;
};

static_assert(request_body_stream<std::ifstream>);
static_assert(request_body_stream<std::istringstream>);
static_assert(!request_body_stream<std::ofstream>);
static_assert(!request_body_stream<std::ostringstream>);
static_assert(!request_body_stream<std::fstream>);
static_assert(!request_body_stream<std::stringstream>);
static_assert(!request_body_stream<bidirectional_stream>);

} // namespace

suite("request_body") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    it("performs a POST request with a JSON body", [] {
        client client{};
        const auto response = client.post(test_server.url("/echo"), nlohmann::json{
            { "message", "Hello from the client" },
            { "answer", 42 }
        });
        const auto json = response.json();

        assert_equal(json.at("message").get<std::string>(), std::string{"Hello from the client"});
        assert_equal(json.at("answer").get<int>(), 42);
        assert_equal(response.header("Content-Type").value(), std::string_view{"application/json"});
    });

    it("performs a POST request with a text body", [] {
        client client{};
        const auto body = std::string{"Text request body"};
        const auto response = client.post(test_server.url("/echo"), body);

        assert_equal(response.text(), std::string_view{body});
        assert_equal(response.header("Content-Type").value(), std::string_view{"text/plain; charset=utf-8"});
    });

    it("performs a PUT request with a byte vector body", [] {
        client client{};
        const auto body = std::vector<std::byte>{
            std::byte{'A'},
            std::byte{0},
            std::byte{'B'}
        };
        const auto response = client.put(test_server.url("/echo"), body);

        assert_equal(response.text(), std::string_view{"A\0B", 3});
        assert_equal(response.header("Content-Type").value(), std::string_view{"application/octet-stream"});
    });

    describe("stream construction", [] {
        it("accepts a stream with an unknown size", [] {
            client client{};
            const auto body = std::string{"Unknown-size streamed request body"};
            auto input = std::istringstream{body};
            const auto response = client.post(test_server.url("/echo"), input);

            assert_equal(response.text(), std::string_view{body});
        });

        it("accepts an explicitly selected bidirectional stream", [] {
            client client{};
            const auto body = std::string{"Bidirectional request body"};
            auto stream = std::stringstream{body};

            const auto response = client.post(test_server.url("/echo"), request_body{stream});

            assert_equal(response.text(), std::string_view{body});
        });
    });

    describe("request method support", [] {
        it("rejects HEAD request bodies", [] {
            client client{};
            const auto body = std::string{"HEAD request body"};

            assert_throw<error>(
                [&client, &body] { return client.request("HEAD", test_server.url("/echo"), body); },
                std::regex{".*HEAD request bodies.*not supported.*"}
            );

            auto input = std::istringstream{body};
            assert_throw<error>(
                [&client, &input, &body] {
                    return client.request("HEAD", test_server.url("/echo"), sized_istream{input, body.size()});
                },
                std::regex{".*HEAD request bodies.*not supported.*"}
            );

            const auto response = client.get(test_server.url("/get"));
            assert_equal(response.status_code(), 200);
        });

        it("supports request bodies for arbitrary HTTP methods", [] {
            client client{};
            const auto body = std::string{"GET request body"};
            auto input = std::istringstream{body};
            const auto response = client.request("GET", test_server.url("/echo"), sized_istream{input, body.size()});

            assert_equal(response.text(), std::string_view{body});
        });
    });

    describe("constructor edge cases", [] {
        it("rejects null C strings", [] {
            assert_throw<std::invalid_argument>(
                [] { return request_body{nullptr}; },
                std::regex{".*must not be null.*"}
            );
        });

        it("creates an explicitly empty request body", [] {
            client client{};
            const auto response = client.post(test_server.url("/echo"), request_body{});

            assert_true(response.text().empty());
        });

        it("preserves embedded null bytes in text views", [] {
            client client{};
            const auto text = std::string{"A\0B", 3};
            const auto response = client.post(test_server.url("/echo"), request_body{std::string_view{text}});

            assert_equal(response.text(), std::string_view{text});
        });

        it("accepts mutable and immutable byte spans", [] {
            client client{};
            auto mutable_bytes = std::array{std::byte{'A'}, std::byte{0}, std::byte{'B'}};
            const auto immutable_bytes = std::array{std::byte{'C'}, std::byte{0}, std::byte{'D'}};

            const auto mutable_response = client.post(
                test_server.url("/echo"),
                request_body{std::span<std::byte>{mutable_bytes}}
            );
            const auto immutable_response = client.post(
                test_server.url("/echo"),
                request_body{std::span<const std::byte>{immutable_bytes}}
            );

            assert_equal(mutable_response.text(), std::string_view{"A\0B", 3});
            assert_equal(immutable_response.text(), std::string_view{"C\0D", 3});
        });
    });
}
