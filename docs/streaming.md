# Streaming

Response streaming writes a successful response body directly to an `std::ostream` instead of retaining it in memory. The returned `streamed_response` contains the final status code and headers.

The client represents the destination as a `response_body`, which is constructed implicitly from ordinary output streams.

```cpp
#include <kaycxx/http.hpp>

using namespace kaycxx::http;

int main() {
    client http_client{};
    auto output = std::ofstream{"archive.zip", std::ios::binary};
    const auto response = http_client.get("https://example.com/archive.zip", output);

    std::cout << "HTTP " << response.status_code() << '\n';
}
```

## Streaming a Download

Pass an output stream after the URL to stream a response without a request body.

Successful response chunks are written as libcurl delivers them. A transport or output error can therefore leave a partial response in the output stream.

## Streaming an Upload

Pass an input stream as the request body. An unsized stream is read until end of input.

```cpp
auto input = std::ifstream{"upload.bin", std::ios::binary};
const auto response = http_client.put("https://example.com/upload", input);
```

Use `sized_istream` when the number of bytes is known:

```cpp
auto input = std::ifstream{"upload.bin", std::ios::binary};
const auto size = std::filesystem::file_size("upload.bin");

const auto response = http_client.put(
    "https://example.com/upload",
    sized_istream{input, size}
);
```

The stream is consumed from its current position. Redirects and authentication retries can require it to seek back to that position. See [Request Bodies] for the stream lifetime and rewind rules.

## Streaming in Both Directions

Place the output stream after the request body to stream upload and download in the same request.

```cpp
auto input = std::ifstream{"request.bin", std::ios::binary};
auto output = std::ofstream{"response.bin", std::ios::binary};

const auto response = http_client.put(
    "https://example.com/process",
    input,
    output,
    {
        .headers = {
            { "Content-Type", "application/octet-stream" }
        }
    }
);
```

## Bidirectional Stream Types

A stream derived from both `std::istream` and `std::ostream` can represent either side of a request. Select the intended role explicitly when passing such a stream as the only body argument:

```cpp
auto stream = std::stringstream{};

const auto upload_response = http_client.post(
    "https://example.com/upload",
    request_body{stream}
);

const auto download_response = http_client.get(
    "https://example.com/download",
    response_body{stream}
);
```

This applies equally to `std::fstream` and custom bidirectional stream classes. Input-only and output-only streams remain unambiguous and do not need an explicit wrapper.

## Error Responses

Only successful 2xx response bodies are written to the output stream. A non-success response body is buffered and exposed through the thrown `response_error`.

```cpp
try {
    const auto response = http_client.get("https://example.com/archive.zip", output);
} catch (const response_error& exception) {
    std::cerr << exception.response().text() << '\n';
}
```

This keeps an HTTP error document out of a destination intended for successful content. The configured `max_buffered_response_size` still applies to the buffered error body. It does not limit a successfully streamed body.

[Request Bodies]: request-bodies.md
