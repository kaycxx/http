# Responses and Errors

HTTP status codes from 200 through 299 return a successful response. Calls that buffer the body return `buffered_response`; calls that write it to an output stream return `streamed_response`.

```cpp
#include <kaycxx/http.hpp>

using namespace kaycxx::http;

int main() {
    client http_client{};
    const auto response = http_client.get("https://example.com/message");

    std::cout << response.status_code() << '\n';
    std::cout << response.text() << '\n';
}
```

## Buffered Responses

Use `status_code()` for the numeric status and `text()` for the body bytes.

On an lvalue response, `text()` returns a non-owning `std::string_view`. It remains valid until the response is destroyed, moved from, or assigned a new value.

Calling `text()` on an expiring response moves the owned string out instead:

```cpp
const auto text = http_client.get("https://example.com/message").text();
```

This makes the common temporary-response expression own its result safely.

Despite its name, `text()` does not inspect `Content-Type` and does not convert character encodings. It exposes the received and content-decoded bytes through `std::string`. Applications that support charsets other than their expected encoding must perform transcoding separately.

## JSON Responses

`json()` parses the buffered body with nlohmann/json and returns an owning JSON value.

```cpp
const auto response = http_client.get("https://example.com/status");
const auto json = response.json();

std::cout << json.at("message").get<std::string>() << '\n';
```

An empty body is represented as JSON null. Invalid JSON throws `nlohmann::json::parse_error` unchanged.

## Response Headers

Every response contains its final status code and final response headers. Intermediate redirect headers are not included. HTTP trailers reported by libcurl are retained with the headers in received order.

Header lookup is ASCII case-insensitive. [Headers and Options] describes lookup of single and repeated values.

## HTTP Response Errors

A status outside the successful 200 through 299 range throws `response_error`. The exception owns the complete buffered response, including its status, body, and headers.

```cpp
try {
    const auto response = http_client.get("https://example.com/missing");
    use(response);
} catch (const response_error& exception) {
    std::cerr << exception.what() << '\n';
    std::cerr << exception.response().status_code() << '\n';
    std::cerr << exception.response().text() << '\n';
}
```

`response_error` derives from `error`. Catch it before a general `error` handler when the HTTP response is relevant.

## Client and Stream Errors

Failures that do not represent a complete HTTP response throw `error`. This includes connection and TLS failures, timeouts, invalid backend configuration, input or output stream error states detected by the client, and exceeding the configured buffered response limit.

Exceptions thrown by user-provided input or output streams are propagated unchanged.

```cpp
try {
    const auto response = http_client.get("https://example.com/data");
    use(response);
} catch (const response_error& exception) {
    handle_http_error(exception.response());
} catch (const error& exception) {
    handle_client_error(exception.what());
}
```

nlohmann/json exceptions are not wrapped in `error`.

[Headers and Options]: headers-and-options.md
