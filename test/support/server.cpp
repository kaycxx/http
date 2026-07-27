// SPDX-FileCopyrightText: 2026 Klaus Reimer <k@ailis.de>
// SPDX-License-Identifier: MIT

#include <support/server.hpp>

#include <nlohmann/json.hpp>

#include <chrono>
#include <format>
#include <stdexcept>
#include <thread>
#include <utility>

namespace kaycxx::http::test {

server::server() {
    const auto echo = [](const httplib::Request& request, httplib::Response& response) {
        const auto content_type = request.get_header_value("Content-Type", "application/octet-stream");
        response.set_content(request.body, content_type);
    };
    const auto url_encoded_form = [](const httplib::Request& request, httplib::Response& response) {
        auto fields = nlohmann::json::object();
        for (const auto& [name, value] : request.params) {
            if (!fields.contains(name)) {
                fields[name] = nlohmann::json::array();
            }
            fields[name].push_back(value);
        }
        response.set_content(nlohmann::json{
            { "content_type", request.get_header_value("Content-Type") },
            { "fields", std::move(fields) }
        }.dump(), "application/json");
    };
    const auto multipart_form = [](const httplib::Request& request, httplib::Response& response) {
        auto parts = nlohmann::json::object();
        const auto append_part = [&parts](
            std::string_view name,
            std::string_view part_name,
            std::string_view filename,
            std::string_view content_type,
            std::string_view content
        ) {
            auto bytes = nlohmann::json::array();
            for (const auto character : content) {
                bytes.push_back(static_cast<unsigned char>(character));
            }
            if (!parts.contains(name)) {
                parts[name] = nlohmann::json::array();
            }
            parts[name].push_back(nlohmann::json{
                { "name", part_name },
                { "filename", filename },
                { "content_type", content_type },
                { "bytes", std::move(bytes) }
            });
        };
        // Normalize the legacy flat multipart representation and the newer split form representation.
        const auto append_parts = [&append_part]<typename Request>(const Request& multipart_request) {
            if constexpr (requires { multipart_request.form.fields; multipart_request.form.files; }) {
                for (const auto& [name, field] : multipart_request.form.fields) {
                    const auto header = field.headers.find("Content-Type");
                    const auto content_type = header == field.headers.end()
                        ? std::string_view{}
                        : std::string_view{header->second};
                    append_part(name, field.name, {}, content_type, field.content);
                }
                for (const auto& [name, file] : multipart_request.form.files) {
                    append_part(name, file.name, file.filename, file.content_type, file.content);
                }
            } else {
                for (const auto& [name, part] : multipart_request.files) {
                    append_part(name, part.name, part.filename, part.content_type, part.content);
                }
            }
        };
        append_parts(request);
        response.set_content(nlohmann::json{
            { "method", request.method },
            { "content_type", request.get_header_value("Content-Type") },
            { "multipart", request.is_multipart_form_data() },
            { "parts", std::move(parts) }
        }.dump(), "application/json");
    };

    server_.Get("/get", [](const httplib::Request&, httplib::Response& response) {
        response.set_header("X-Response-Header", "response value");
        response.set_header("X-Multiple-Header", "first value");
        response.set_header("X-Multiple-Header", "second value");
        response.set_content("Hello from the test server", "text/plain");
    });
    server_.Get("/header", [](const httplib::Request& request, httplib::Response& response) {
        response.set_content(request.get_header_value("X-Test-Header"), "text/plain");
    });
    server_.Get("/empty-header", [](const httplib::Request& request, httplib::Response& response) {
        response.set_content(request.has_header("X-Empty-Header") ? "present" : "missing", "text/plain");
    });
    server_.Get("/error", [](const httplib::Request&, httplib::Response& response) {
        response.status = 400;
        response.set_header("X-Error-Header", "error value");
        response.set_content(R"({"error":"Invalid request"})", "application/json");
    });
    server_.Post("/redirect", [](const httplib::Request&, httplib::Response& response) {
        response.set_redirect("/echo", 307);
    });
    server_.Post("/redirect-multipart", [](const httplib::Request&, httplib::Response& response) {
        response.set_redirect("/multipart-form", 307);
    });
    server_.Put("/redirect-multipart-301", [](const httplib::Request&, httplib::Response& response) {
        response.set_redirect("/multipart-form", 301);
    });
    server_.Put("/redirect-multipart-302", [](const httplib::Request&, httplib::Response& response) {
        response.set_redirect("/multipart-form", 302);
    });
    server_.Get("/redirect-temporary", [](const httplib::Request&, httplib::Response& response) {
        response.set_redirect("/echo", 302);
    });
    server_.Put("/redirect-see-other", [](const httplib::Request&, httplib::Response& response) {
        response.set_redirect("/method", 303);
    });
    server_.Delete("/redirect-see-other", [](const httplib::Request&, httplib::Response& response) {
        response.set_redirect("/method", 303);
    });
    server_.Get("/redirect-with-body", [](const httplib::Request&, httplib::Response& response) {
        response.set_header("X-Intermediate-Header", "intermediate value");
        response.set_redirect("/get", 302);
        response.set_content("Intermediate redirect body", "text/plain");
    });
    server_.Get("/slow", [](const httplib::Request&, httplib::Response& response) {
        std::this_thread::sleep_for(std::chrono::milliseconds{200});
        response.set_content("Slow response", "text/plain");
    });
    const auto method = [](const httplib::Request& request, httplib::Response& response) {
        response.set_content(request.method, "text/plain");
    };
    server_.Get("/method", method);
    server_.Options("/method", method);
    server_.Get("/echo", echo);
    server_.Post("/echo", echo);
    server_.Put("/echo", echo);
    server_.Delete("/echo", echo);
    server_.Post("/url-encoded-form", url_encoded_form);
    server_.Get("/multipart-form", multipart_form);
    server_.Post("/multipart-form", multipart_form);
    server_.Put("/multipart-form", multipart_form);
}

server::~server() {
    stop();
}

void server::start() {
    if (thread_.joinable()) {
        throw std::logic_error{"Test HTTP server is already running"};
    }

    port_ = server_.bind_to_any_port("127.0.0.1");
    if (port_ <= 0) {
        throw std::runtime_error{"Unable to bind test HTTP server"};
    }

    thread_ = std::jthread{
        [this] {
            server_.listen_after_bind();
        }
    };
    server_.wait_until_ready();
}

void server::stop() {
    if (!thread_.joinable()) {
        return;
    }

    server_.stop();
    thread_.join();
    port_ = 0;
}

std::string server::url(std::string_view path) const {
    if (port_ <= 0) {
        throw std::logic_error{"Test HTTP server is not running"};
    }
    if (path.empty() || path.front() != '/') {
        throw std::invalid_argument{"Test HTTP server endpoint path must start with '/'"};
    }
    return std::format("http://127.0.0.1:{}{}", port_, path);
}

} // namespace kaycxx::http::test
