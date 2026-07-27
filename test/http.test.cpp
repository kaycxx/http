// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/assert.hpp>
#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <regex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace kaycxx::assert;
using namespace kaycxx::http;
using namespace kaycxx::test;

namespace {

/** Stream buffer throwing when data is read from it. */
class throwing_input_buffer final : public std::streambuf {
private:
    /**
     * Throws instead of reading data.
     *
     * @throws std::logic_error  Always.
     */
    std::streamsize xsgetn(char*, std::streamsize) override {
        throw std::logic_error{"Input stream failed"};
    }
};

/** Stream buffer throwing when data is written to it. */
class throwing_output_buffer final : public std::streambuf {
private:
    /**
     * Throws instead of writing data.
     *
     * @throws std::logic_error  Always.
     */
    std::streamsize xsputn(const char*, std::streamsize) override {
        throw std::logic_error{"Output stream failed"};
    }
};

/** Stream which can be used for both input and output. */
class bidirectional_stream : public std::iostream {
public:
    using std::iostream::iostream;
};

/**
 * Converts text bytes to integer values for comparison with the test server response.
 *
 * @param value  Text bytes.
 * @returns Integer byte values.
 */
[[nodiscard]] std::vector<int> byte_values(std::string_view value) {
    auto result = std::vector<int>{};
    result.reserve(value.size());
    for (const auto character : value) {
        result.push_back(static_cast<unsigned char>(character));
    }
    return result;
}

/**
 * Converts binary bytes to integer values for comparison with the test server response.
 *
 * @param value  Binary bytes.
 * @returns Integer byte values.
 */
[[nodiscard]] std::vector<int> byte_values(std::span<const std::byte> value) {
    auto result = std::vector<int>{};
    result.reserve(value.size());
    for (const auto byte : value) {
        result.push_back(std::to_integer<int>(byte));
    }
    return result;
}

/**
 * Converts JSON integer byte values to a string.
 *
 * @param value  JSON byte array.
 * @returns String containing the represented bytes.
 */
[[nodiscard]] std::string byte_text(const nlohmann::json& value) {
    auto result = std::string{};
    result.reserve(value.size());
    for (const auto& byte : value) {
        result.push_back(static_cast<char>(byte.get<unsigned char>()));
    }
    return result;
}

/** Whether a stream is unambiguously accepted as a buffered request body. */
template<typename stream_type>
concept request_body_stream = requires(client& http, std::string_view url, stream_type& stream) {
    { http.post(url, stream) } -> std::same_as<buffered_response>;
};

/** Whether a stream is unambiguously accepted as a streamed response body. */
template<typename stream_type>
concept response_body_stream = requires(client& http, std::string_view url, stream_type& stream) {
    { http.post(url, stream) } -> std::same_as<streamed_response>;
};

static_assert(request_body_stream<std::ifstream>);
static_assert(request_body_stream<std::istringstream>);
static_assert(!response_body_stream<std::ifstream>);
static_assert(!response_body_stream<std::istringstream>);
static_assert(response_body_stream<std::ofstream>);
static_assert(response_body_stream<std::ostringstream>);
static_assert(!request_body_stream<std::ofstream>);
static_assert(!request_body_stream<std::ostringstream>);
static_assert(!request_body_stream<std::fstream>);
static_assert(!response_body_stream<std::fstream>);
static_assert(!request_body_stream<std::stringstream>);
static_assert(!response_body_stream<std::stringstream>);
static_assert(!request_body_stream<bidirectional_stream>);
static_assert(!response_body_stream<bidirectional_stream>);

} // namespace

suite("client") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    it("performs a GET request", [] {
        client client{};
        const auto response = client.get(test_server.url("/get"));

        assert_equal(response.status_code(), 200);
        assert_equal(response.text(), std::string_view{"Hello from the test server"});
        const auto content_type = response.header("content-type");
        assert_true(content_type.has_value());
        assert_equal(*content_type, std::string_view{"text/plain"});
        const auto response_header = response.header("x-response-header");
        assert_true(response_header.has_value());
        assert_equal(*response_header, std::string_view{"response value"});
        assert_false(response.header("X-Missing-Header").has_value());

        auto multiple_values = response.headers("x-multiple-header");
        std::ranges::sort(multiple_values);
        assert_equal(multiple_values.size(), std::size_t{2});
        assert_equal(multiple_values[0], std::string{"first value"});
        assert_equal(multiple_values[1], std::string{"second value"});
    });

    it("rejects requests on a move-constructed source client", [] {
        client source{};
        client destination{std::move(source)};
        auto output = std::ostringstream{};

        assert_throw<error>(
            [&source] { return source.request("GET", "https://example.com"); },
            std::regex{".*moved from.*"}
        );
        assert_throw<error>(
            [&source] { return source.request("POST", "https://example.com", "body"); },
            std::regex{".*moved from.*"}
        );
        assert_throw<error>(
            [&source, &output] { return source.request("GET", "https://example.com", output); },
            std::regex{".*moved from.*"}
        );
        assert_throw<error>(
            [&source, &output] { return source.request("POST", "https://example.com", "body", output); },
            std::regex{".*moved from.*"}
        );

        assert_equal(destination.get(test_server.url("/get")).status_code(), 200);
    });

    it("rejects requests on a move-assigned source client", [] {
        client source{};
        client destination{};
        destination = std::move(source);

        assert_throw<error>(
            [&source] { return source.get("https://example.com"); },
            std::regex{".*moved from.*"}
        );
        assert_equal(destination.get(test_server.url("/get")).status_code(), 200);
    });

    it("performs an arbitrary HEAD request", [] {
        client client{};
        const auto response = client.request("HEAD", test_server.url("/get"));

        assert_equal(response.status_code(), 200);
        assert_equal(response.text(), std::string_view{});
        assert_equal(response.header("X-Response-Header").value(), std::string_view{"response value"});
    });

    it("performs a valid custom request method", [] {
        client client{};
        const auto response = client.request("OPTIONS", test_server.url("/method"));

        assert_equal(response.text(), std::string_view{"OPTIONS"});
    });

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

    it("moves text out of a temporary GET response", [] {
        client client{};
        const auto text = client.get(test_server.url("/get")).text();

        static_assert(std::is_same_v<decltype(text), const std::string>);
        assert_equal(text, std::string{"Hello from the test server"});
    });

    it("moves a header out of a temporary GET response", [] {
        client client{};
        const auto value = client.get(test_server.url("/get")).header("X-Response-Header");

        static_assert(std::is_same_v<decltype(value), const std::optional<std::string>>);
        assert_true(value.has_value());
        assert_equal(*value, std::string{"response value"});
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

    it("sends a URL-encoded form", [] {
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

    it("sends an empty URL-encoded form as a present request body", [] {
        client client{};
        const auto response = client.post(test_server.url("/echo"), url_encoded_form{});

        assert_true(response.text().empty());
        assert_equal(
            response.header("Content-Type").value(),
            std::string_view{"application/x-www-form-urlencoded"}
        );
    });

    it("sends a multipart form with buffered and streamed parts", [] {
        client client{};
        const auto text = std::string{"Hello\r\n+ &= %"};
        const auto binary = std::vector<std::byte>{
            std::byte{0x00},
            std::byte{0x0d},
            std::byte{0x0a},
            std::byte{0x7f},
            std::byte{0x80},
            std::byte{0xff}
        };
        const auto metadata = nlohmann::json{
            { "message", "Hello from JSON" },
            { "answer", 42 }
        };
        auto stream = std::istringstream{"skip:streamed part"};
        stream.seekg(5);
        const auto sized_text = std::string{"sized streamed part"};
        auto sized_stream = std::istringstream{sized_text};
        const auto encoded = url_encoded_form{
            { "nested", "form value" }
        };

        auto form = multipart_form{
            {
                .name = "text",
                .body = text
            },
            {
                .name = "binary",
                .body = binary,
                .filename = "payload.bin"
            },
            {
                .name = "metadata",
                .body = metadata,
                .filename = "metadata.json"
            },
            {
                .name = "stream",
                .body = stream,
                .filename = "stream.txt",
                .content_type = "text/plain"
            },
            {
                .name = "sized_stream",
                .body = sized_istream{sized_stream, sized_text.size()},
                .filename = "sized.bin"
            },
            {
                .name = "encoded",
                .body = encoded
            },
            {
                .name = "header_type",
                .body = "custom type",
                .content_type = "application/ignored",
                .headers = {
                    { "Content-Type", "application/custom" }
                }
            },
            {
                .name = "empty_header_type",
                .body = "empty type",
                .headers = {
                    { "Content-Type", "" }
                }
            },
            {
                .name = "tag",
                .body = "first"
            }
        };
        form.add({
            .name = "tag",
            .body = "second"
        });

        const auto response = client.post(test_server.url("/multipart-form"), form).json();
        const auto content_type = response.at("content_type").get<std::string>();
        const auto& parts = response.at("parts");

        assert_true(response.at("multipart").get<bool>());
        assert_true(std::string_view{content_type}.starts_with("multipart/form-data; boundary="));
        assert_equal(response.at("method").get<std::string>(), std::string{"POST"});

        const auto& text_part = parts.at("text").at(0);
        assert_equal(text_part.at("name").get<std::string>(), std::string{"text"});
        assert_equal(text_part.at("filename").get<std::string>(), std::string{});
        assert_equal(
            text_part.at("content_type").get<std::string>(),
            std::string{"text/plain; charset=utf-8"}
        );
        assert_equal(text_part.at("bytes").get<std::vector<int>>(), byte_values(text));

        const auto& binary_part = parts.at("binary").at(0);
        assert_equal(binary_part.at("filename").get<std::string>(), std::string{"payload.bin"});
        assert_equal(binary_part.at("content_type").get<std::string>(), std::string{"application/octet-stream"});
        assert_equal(binary_part.at("bytes").get<std::vector<int>>(), byte_values(binary));

        const auto& metadata_part = parts.at("metadata").at(0);
        assert_equal(metadata_part.at("filename").get<std::string>(), std::string{"metadata.json"});
        assert_equal(metadata_part.at("content_type").get<std::string>(), std::string{"application/json"});
        assert_equal(nlohmann::json::parse(byte_text(metadata_part.at("bytes"))), metadata);

        const auto& stream_part = parts.at("stream").at(0);
        assert_equal(stream_part.at("filename").get<std::string>(), std::string{"stream.txt"});
        assert_equal(stream_part.at("content_type").get<std::string>(), std::string{"text/plain"});
        assert_equal(byte_text(stream_part.at("bytes")), std::string{"streamed part"});

        const auto& sized_part = parts.at("sized_stream").at(0);
        assert_equal(sized_part.at("filename").get<std::string>(), std::string{"sized.bin"});
        assert_equal(sized_part.at("content_type").get<std::string>(), std::string{"application/octet-stream"});
        assert_equal(byte_text(sized_part.at("bytes")), sized_text);

        const auto& encoded_part = parts.at("encoded").at(0);
        assert_equal(
            encoded_part.at("content_type").get<std::string>(),
            std::string{"application/x-www-form-urlencoded"}
        );
        assert_equal(byte_text(encoded_part.at("bytes")), std::string{"nested=form+value"});

        const auto& header_type_part = parts.at("header_type").at(0);
        assert_equal(
            header_type_part.at("content_type").get<std::string>(),
            std::string{"application/custom"}
        );
        assert_equal(byte_text(header_type_part.at("bytes")), std::string{"custom type"});

        const auto& empty_header_type_part = parts.at("empty_header_type").at(0);
        assert_equal(empty_header_type_part.at("content_type").get<std::string>(), std::string{});
        assert_equal(byte_text(empty_header_type_part.at("bytes")), std::string{"empty type"});

        assert_equal(byte_text(parts.at("tag").at(0).at("bytes")), std::string{"first"});
        assert_equal(byte_text(parts.at("tag").at(1).at("bytes")), std::string{"second"});
    });

    it("sends a multipart form with a PUT request", [] {
        client client{};
        const auto response = client.put(test_server.url("/multipart-form"), multipart_form{
            {
                .name = "message",
                .body = "PUT multipart body"
            }
        }).json();

        assert_equal(response.at("method").get<std::string>(), std::string{"PUT"});
        assert_equal(
            byte_text(response.at("parts").at("message").at(0).at("bytes")),
            std::string{"PUT multipart body"}
        );
    });

    it("sends a multipart form with a GET request", [] {
        client client{};
        const auto response = client.get(test_server.url("/multipart-form"), multipart_form{
            {
                .name = "message",
                .body = "GET multipart body"
            }
        }).json();

        assert_equal(response.at("method").get<std::string>(), std::string{"GET"});
        assert_equal(
            byte_text(response.at("parts").at("message").at(0).at("bytes")),
            std::string{"GET multipart body"}
        );
    });

    it("retains a multipart PUT after moved and found redirects", [] {
        client client{};
        for (const auto path : { "/redirect-multipart-301", "/redirect-multipart-302" }) {
            const auto response = client.put(
                test_server.url(path),
                multipart_form{
                    {
                        .name = "message",
                        .body = "redirected multipart body"
                    }
                },
                request_options{
                    .follow_redirects = true
                }
            ).json();

            assert_equal(response.at("method").get<std::string>(), std::string{"PUT"});
            assert_equal(
                byte_text(response.at("parts").at("message").at(0).at("bytes")),
                std::string{"redirected multipart body"}
            );
        }
    });

    it("changes a multipart PUT to GET after a see-other redirect", [] {
        client client{};
        const auto response = client.put(
            test_server.url("/redirect-see-other"),
            multipart_form{
                {
                    .name = "message",
                    .body = "redirected multipart body"
                }
            },
            request_options{
                .follow_redirects = true
            }
        );

        assert_equal(response.text(), std::string_view{"GET"});
    });

    it("rejects an empty multipart form", [] {
        client client{};

        assert_throw<error>(
            [&client] { return client.post(test_server.url("/multipart-form"), multipart_form{}); },
            std::regex{".*multipart form.*at least one part.*"}
        );
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

    it("rejects invalid control characters in a multipart content type", [] {
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

    it("escapes multipart names and filenames", [] {
        client client{};
        const auto form = multipart_form{
            {
                .name = "field\"\r\nX-Injected: yes",
                .body = "body",
                .filename = "file\"\r\nX-Injected: yes.txt"
            }
        };

        const auto response = client.post(test_server.url("/multipart-form"), form).json();
        const auto& parts = response.at("parts");
        const auto& part = parts.at("field%22%0D%0AX-Injected: yes").at(0);

        assert_equal(parts.size(), std::size_t{1});
        assert_equal(
            part.at("filename").get<std::string>(),
            std::string{"file%22%0D%0AX-Injected: yes.txt"}
        );
    });

    it("performs a POST request with a text body", [] {
        client client{};
        const auto body = std::string{"Text request body"};
        const auto response = client.post(test_server.url("/echo"), body);

        assert_equal(response.text(), std::string_view{body});
        assert_equal(response.header("Content-Type").value(), std::string_view{"text/plain; charset=utf-8"});
    });

    it("performs a POST request without a body", [] {
        client client{};
        const auto response = client.post(test_server.url("/echo"));

        assert_true(response.text().empty());
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

    it("performs DELETE requests with and without a body", [] {
        client client{};
        const auto body = std::string{"DELETE request body"};

        const auto response = client.del(test_server.url("/echo"), body);
        assert_equal(response.text(), std::string_view{body});

        const auto empty_response = client.del(test_server.url("/echo"));
        assert_true(empty_response.text().empty());
    });

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

    it("sends structurally valid request header values", [] {
        client client{};
        const auto value = std::string{"Grüße\tvalue: still data"};
        const auto response = client.get(test_server.url("/header"), request_options{
            .headers = {
                { "X-Test-Header", value }
            }
        });

        assert_equal(response.text(), std::string_view{value});
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

    it("rejects invalid multipart part headers", [] {
        client client{};
        const auto invalid_headers = std::vector<header>{
            { "X Part", "value" },
            { "X-Part", "value\r\nX-Injected: true" }
        };

        for (const auto& invalid_header : invalid_headers) {
            const auto form = multipart_form{
                {
                    .name = "value",
                    .body = "body",
                    .headers = { invalid_header }
                }
            };

            assert_throw<error>(
                [&client, &form] { return client.post(test_server.url("/multipart-form"), form); },
                std::regex{".*HTTP multipart part header (name|value).*"}
            );
        }
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

    it("uses an explicitly selected response role for a bidirectional stream", [] {
        client client{};
        auto stream = std::stringstream{};

        const auto response = client.get(test_server.url("/get"), response_body{stream});

        assert_equal(response.status_code(), 200);
        assert_equal(stream.str(), std::string{"Hello from the test server"});
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

    it("reports upload and download progress", [] {
        client client{};
        const auto body = std::string{"Progress test body"};
        auto progress = transfer_progress{};

        const auto response = client.post(test_server.url("/echo"), body, request_options{
            .progress = [&progress](const auto& current) {
                progress = current;
            }
        });

        const auto size = std::uint64_t{body.size()};
        assert_equal(response.text(), std::string_view{body});
        assert_equal(progress.uploaded, size);
        assert_equal(progress.upload_total, size);
        assert_equal(progress.downloaded, size);
        assert_equal(progress.download_total, size);
    });

    it("reports zero progress totals unchanged", [] {
        client client{};
        auto progress = transfer_progress{
            .downloaded = 1,
            .download_total = 1,
            .uploaded = 1,
            .upload_total = 1
        };

        const auto response = client.post(test_server.url("/echo"), "", request_options{
            .progress = [&progress](const auto& current) {
                progress = current;
            }
        });

        assert_true(response.text().empty());
        assert_equal(progress.downloaded, std::uint64_t{});
        assert_equal(progress.download_total, std::uint64_t{});
        assert_equal(progress.uploaded, std::uint64_t{});
        assert_equal(progress.upload_total, std::uint64_t{});
    });

    it("propagates progress callback exceptions", [] {
        client client{};

        assert_throw<std::logic_error>(
            [&client] {
                return client.get(test_server.url("/get"), request_options{
                    .progress = [](const auto&) {
                        throw std::logic_error{"Progress callback failed"};
                    }
                });
            },
            std::regex{"Progress callback failed"}
        );
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

    it("limits buffered successful response bodies", [] {
        client client{};

        assert_throw<error>(
            [&client] {
                return client.get(test_server.url("/get"), request_options{
                    .max_buffered_response_size = 4
                });
            },
            std::regex{".*buffer limit.*"}
        );
    });

    it("can disable the buffered response body limit", [] {
        client client{};
        const auto response = client.get(test_server.url("/get"), request_options{
            .max_buffered_response_size = std::nullopt
        });

        assert_equal(response.text(), std::string_view{"Hello from the test server"});
    });

    it("limits buffered error response bodies", [] {
        client client{};

        assert_throw<error>(
            [&client] {
                return client.get(test_server.url("/error"), request_options{
                    .max_buffered_response_size = 4
                });
            },
            std::regex{".*buffer limit.*"}
        );
    });

    it("does not limit streamed successful response bodies", [] {
        client client{};
        auto output = std::ostringstream{};
        const auto response = client.get(test_server.url("/get"), output, request_options{
            .max_buffered_response_size = 4
        });

        assert_equal(response.status_code(), 200);
        assert_equal(output.str(), std::string{"Hello from the test server"});
    });

    it("limits buffered error bodies when successful responses are streamed", [] {
        client client{};
        auto output = std::ostringstream{};

        assert_throw<error>(
            [&client, &output] {
                return client.get(test_server.url("/error"), output, request_options{
                    .max_buffered_response_size = 4
                });
            },
            std::regex{".*buffer limit.*"}
        );
        assert_true(output.str().empty());
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

    it("streams a request body with a known size", [] {
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

    it("streams a POST request body with an unknown size", [] {
        client client{};
        const auto body = std::string{"Unknown-size streamed request body"};
        auto input = std::istringstream{body};
        const auto response = client.post(test_server.url("/echo"), input);

        assert_equal(response.text(), std::string_view{body});
    });

    it("uses an explicitly selected request role for a bidirectional stream", [] {
        client client{};
        const auto body = std::string{"Bidirectional request body"};
        auto stream = std::stringstream{body};

        const auto response = client.post(test_server.url("/echo"), request_body{stream});

        assert_equal(response.text(), std::string_view{body});
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

    it("supports a request body for arbitrary HTTP methods", [] {
        client client{};
        const auto body = std::string{"GET request body"};
        auto input = std::istringstream{body};
        const auto response = client.request("GET", test_server.url("/echo"), sized_istream{input, body.size()});

        assert_equal(response.text(), std::string_view{body});
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

    it("provides error response details", [] {
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
}

suite("request_body") {
    it("rejects null C strings", [] {
        assert_throw<std::invalid_argument>(
            [] { return request_body{nullptr}; },
            std::regex{".*must not be null.*"}
        );
    });
}

suite("request_options") {
    it("provides the request defaults", [] {
        const request_options options{};

        assert_equal(options.follow_redirects, std::optional<bool>{});
        assert_equal(options.max_redirects, std::optional<std::size_t>{});
        assert_equal(options.connect_timeout, std::optional<std::chrono::milliseconds>{});
        assert_equal(options.request_timeout, std::optional<std::chrono::milliseconds>{});
        assert_false(static_cast<bool>(options.progress));
        assert_equal(options.max_buffered_response_size, std::optional<std::size_t>{64 * 1024 * 1024});
    });
}

suite("buffered_response") {
    it("provides status, text and JSON", [] {
        const buffered_response response{200, R"({"success":true})"};

        assert_equal(response.status_code(), 200);
        assert_equal(response.text(), std::string_view{R"({"success":true})"});
        assert_true(response.json().at("success").get<bool>());
    });

    it("represents an empty body as JSON null", [] {
        const buffered_response response{204, ""};

        assert_true(response.json().is_null());
    });

    it("propagates JSON parse errors", [] {
        const buffered_response response{200, "{"};

        assert_throw<nlohmann::json::parse_error>(
            [&response] { return response.json(); },
            std::regex{".*json\\.exception\\.parse_error.*"}
        );
    });
}

suite("error") {
    it("provides its message", [] {
        const error exception{"request failed"};

        assert_equal(std::string_view{exception.what()}, std::string_view{"request failed"});
    });
}

suite("response_error") {
    it("provides its message and response", [] {
        const response_error exception{"not found", buffered_response{404, R"({"error":"Not found"})"}};
        const auto& response = exception.response();

        assert_equal(std::string_view{exception.what()}, std::string_view{"not found"});
        assert_equal(response.status_code(), 404);
        assert_equal(response.text(), std::string_view{R"({"error":"Not found"})"});
        assert_equal(response.json().at("error").get<std::string>(), std::string{"Not found"});
    });

    it("represents an empty body as JSON null", [] {
        const response_error exception{"no content", buffered_response{500, ""}};

        assert_true(exception.response().json().is_null());
    });

    it("moves text out of an expiring response error", [] {
        const auto text = response_error{"not found", buffered_response{404, "response body"}}.response().text();

        static_assert(std::is_same_v<decltype(text), const std::string>);
        assert_equal(text, std::string{"response body"});
    });

    it("propagates JSON parse errors", [] {
        const response_error exception{"invalid response", buffered_response{500, "{"}};

        assert_throw<nlohmann::json::parse_error>(
            [&exception] { return exception.response().json(); },
            std::regex{".*json\\.exception\\.parse_error.*"}
        );
    });
}

suite("streamed_response") {
    it("provides its status code", [] {
        const streamed_response response{206};

        assert_equal(response.status_code(), 206);
    });
}
