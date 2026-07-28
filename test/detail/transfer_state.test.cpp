// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <kaycxx/http.hpp>
#include <kaycxx/test.hpp>

#include <cstddef>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace kaycxx::http;
using namespace kaycxx::test;

suite("detail::transfer_state") {
    static kaycxx::http::test::server test_server{};

    before_all([] {
        test_server.start();
    });

    after_all([] {
        test_server.stop();
    });

    describe("report_progress", [] {
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

        it("reports zero totals unchanged", [] {
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

        it("propagates callback exceptions", [] {
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
    });

    describe("write_body", [] {
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
                .max_buffered_response_size = 0
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
    });
}
