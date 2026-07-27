# Request Bodies

Request bodies are represented by `request_body`. The type is constructed implicitly from common text, binary, stream, JSON, and form values, so callers normally pass the value directly. All methods except HEAD accept a request body.

```cpp
#include <kaycxx/http.hpp>

using namespace kaycxx::http;

int main() {
    client http_client{};
    auto text = std::string{"Hello from the client"};
    const auto response = http_client.post("https://example.com/echo", text);

    return response.status_code() == 200 ? 0 : 1;
}
```

## Text Bodies

Pass a string, string view, or null-terminated string to send text. The default content type is `text/plain; charset=utf-8`.

The library does not transcode the string. Its bytes are sent unchanged. The default charset therefore describes the expected encoding of the supplied text; callers are responsible for providing UTF-8 when that default is used.

## JSON Bodies

Pass a `nlohmann::json` value to serialize it with `dump()` and send it as `application/json`.

```cpp
const auto response = http_client.post("https://example.com/messages", nlohmann::json{
    { "message", "Hello" },
    { "answer", 42 }
});
```

Serialization errors from nlohmann/json are propagated unchanged.

## URL-Encoded Forms

Use `url_encoded_form` for `application/x-www-form-urlencoded` data. Field names and values are owned by the form. Their insertion order and duplicate names are preserved.

```cpp
auto form = url_encoded_form{
    { "name", "Ada Lovelace" },
    { "tag", "one" },
    { "tag", "two" }
};
form.add("message", "Hello world");

const auto response = http_client.post("https://example.com/form", form);
```

Names and values are encoded byte by byte. Spaces become `+`; other bytes outside the form-safe ASCII set become `%HH`. The library does not transcode strings, so callers are responsible for supplying the character encoding expected by the server.

## Multipart Forms

Use `multipart_form` for `multipart/form-data`. The form must contain at least one part. A part accepts the same text, binary, stream, sized stream, JSON, or URL-encoded body values as a regular request. A multipart form cannot itself be nested as a part body.

```cpp
auto input = std::ifstream{"report.pdf", std::ios::binary};

const auto response = http_client.post("https://example.com/upload", multipart_form{
    {
        .name = "message",
        .body = "Hello"
    },
    {
        .name = "attachment",
        .body = sized_istream{
            input,
            std::filesystem::file_size("report.pdf")
        },
        .filename = "report.pdf",
        .content_type = "application/pdf",
        .headers = {
            { "Content-ID", "document" }
        }
    }
});
```

libcurl generates the multipart boundary and the complete outer `Content-Type` header, which cannot be overridden through request options. Each part receives the default content type of its body kind unless `content_type` or a `Content-Type` part header overrides it. When both are specified, the part header takes precedence. `filename` controls the filename reported to the server; omitting it creates a regular form field. Additional headers apply only to that part. `Content-Disposition` is generated from `name` and `filename` and cannot be supplied as an additional part header.

The form owns its part names, filenames, content types, and headers. Each part's `body` remains a non-owning `request_body`, so referenced strings, byte containers, JSON values, URL-encoded forms, and streams must remain alive until the synchronous request returns. Stream parts are read from their current positions. Seekable streams can be rewound when libcurl needs to resend them; resending a non-seekable stream can fail.

## Binary Bodies

Binary data can be passed as a byte span or byte vector. Its default content type is `application/octet-stream`.

```cpp
auto data = std::vector<std::byte>{
    std::byte{0x01},
    std::byte{0x02},
    std::byte{0x03}
};

const auto response = http_client.put("https://example.com/data", data);
```

## Input Streams

An `std::istream` sends data from its current position until end of input. Wrap it in `sized_istream` when the number of bytes is known.

```cpp
auto input = std::ifstream{"upload.bin", std::ios::binary};

const auto response = http_client.put(
    "https://example.com/upload",
    sized_istream{input, std::filesystem::file_size("upload.bin")}
);
```

For an unsized stream, libcurl selects the appropriate unknown-length transfer mechanism for the negotiated HTTP version.

The initial stream position is retained. When libcurl needs to resend the body for a redirect or authentication retry, a seekable stream is rewound to that position. Resending a non-seekable stream can fail.

[Streaming] shows upload and response streaming together.

## Content-Type Overrides

An explicitly configured `Content-Type` header takes precedence over the default selected for the body type.

```cpp
const auto response = http_client.post(
    "https://example.com/document",
    "<message>Hello</message>",
    request_options{
        .headers = {
            { "Content-Type", "application/xml" }
        }
    }
);
```

## Ownership and Lifetime

`request_body` is a non-owning descriptor. Referenced strings, byte containers, streams, JSON values, and forms must remain alive until the synchronous request returns. Passing a value directly to `client` is normally safe because the request completes before the full expression ends.

Do not retain a request body that refers to a short-lived value:

```cpp
request_body body{};

{
    auto text = std::string{"temporary"};
    body = request_body{text};
}

// Invalid: body now refers to the destroyed string.
http_client.post("https://example.com/echo", body);
```

An absent body and a present body with zero bytes are distinct request-body states. This matters for method selection and automatic `Content-Type` headers.

[Streaming]: streaming.md
