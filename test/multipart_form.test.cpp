// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <cstddef>
#include <regex>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace kaycxx::http;
using namespace kaycxx::test;

namespace {

/** Converts text bytes to integer values. */
[[nodiscard]] std::vector<int> byte_values(std::string_view value) {
    auto result = std::vector<int>{};
    result.reserve(value.size());
    for (const auto character : value) {
        result.push_back(static_cast<unsigned char>(character));
    }
    return result;
}

/** Converts binary bytes to integer values. */
[[nodiscard]] std::vector<int> byte_values(std::span<const std::byte> value) {
    auto result = std::vector<int>{};
    result.reserve(value.size());
    for (const auto byte : value) {
        result.push_back(std::to_integer<int>(byte));
    }
    return result;
}

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

suite("multipart_form") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
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

    it("rejects an empty multipart form", [] {
        client client{};

        assert_throw<error>(
            [&client] { return client.post(test_server.url("/multipart-form"), multipart_form{}); },
            std::regex{".*multipart form.*at least one part.*"}
        );
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

    describe("request methods", [] {
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
    });

    describe("redirect handling", [] {
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
    });

}
