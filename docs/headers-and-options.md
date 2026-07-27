# Headers and Options

`request_options` is an aggregate intended for designated initialization. Its defaults leave the libcurl redirect and timeout defaults unchanged and provide finite response buffering.

```cpp
#include <kaycxx/http.hpp>

using namespace kaycxx::http;

int main() {
    client http_client{};
    const auto options = request_options{
        .headers = {
            { "Accept", "application/json" },
            { "X-Request-Id", "example-42" }
        },
        .follow_redirects = true,
        .max_redirects = 5,
        .connect_timeout = std::chrono::seconds{5},
        .request_timeout = std::chrono::minutes{1},
        .max_buffered_response_size = 128 * 1024 * 1024
    };

    const auto response = http_client.get("https://example.com/data", options);

    return response.status_code() == 200 ? 0 : 1;
}
```

## Request Headers

Request header names and values are owning `std::string` values. A header with an empty value is sent as a present header with no value.

```cpp
const auto response = http_client.get("https://example.com/data", request_options{
    .headers = {
        { "X-Empty-Header", "" }
    }
});
```

Header names use HTTP token syntax. Header values allow tabs, visible characters, and non-ASCII bytes, but reject other control characters. These restrictions prevent malformed headers and additional injected wire headers without interpreting header-specific value formats.

`Content-Length` and `Transfer-Encoding` are managed by the client and cannot be configured manually. An explicitly configured `Content-Type` overrides the automatic value selected for a non-multipart request body. For a multipart request, libcurl manages the outer `Content-Type` including its boundary.

## Response Headers

`header()` returns the first value for a name. On an lvalue response it returns an optional non-owning string view.

```cpp
if (const auto content_type = response.header("Content-Type")) {
    std::cout << *content_type << '\n';
}
```

`headers(name)` copies and returns every value for a repeated header. The parameterless `headers()` returns all headers in received order.

```cpp
for (const auto& cookie : response.headers("Set-Cookie")) {
    std::cout << cookie << '\n';
}

for (const auto& response_header : response.headers()) {
    std::cout << response_header.name << ": " << response_header.value << '\n';
}
```

All name comparisons are ASCII case-insensitive.

## Redirects

Enable redirect following explicitly:

```cpp
const auto response = http_client.get("https://example.com/redirect", request_options{
    .follow_redirects = true,
    .max_redirects = 10
});
```

Both redirect options are optional. By default neither is sent to libcurl, which disables redirect following and uses a limit of 30 redirects if following is enabled. An explicitly configured `max_redirects` is sent independently of `follow_redirects`, although it only has an effect when redirect following is enabled.

Redirect status codes select the subsequent method according to normal libcurl rules. A 303 switches to GET except after HEAD. A 307 or 308 retains the method and request body. POST requests normally switch to GET after 301 or 302.

Custom request headers are reused after redirects and can therefore be sent to a different origin. libcurl withholds `Authorization` and explicitly configured `Cookie` headers after an origin change, but arbitrary sensitive headers receive no such protection. Enable redirects with care when options contain credentials in custom header names.

## Timeouts

`connect_timeout` limits the connection phase. `request_timeout` limits the complete transfer, including the connection phase. Both are optional `std::chrono::milliseconds` values and accept compatible chrono durations.

By default neither option is sent to libcurl. Its built-in defaults therefore apply: 300 seconds for the connection phase and no timeout for the complete transfer.

Timeout failures throw `error`.

## Transfer Progress

`progress` receives the current upload and download progress reported by libcurl. `download_total` and `upload_total` contain the expected total number of bytes. A value of `0` means that the total is unknown, the direction is unused or the transfer is empty; libcurl does not distinguish these cases in progress reports.

```cpp
const auto response = http_client.put("https://example.com/data", input, output, request_options{
    .progress = [](const transfer_progress& progress) {
        std::cout
            << "Upload: " << progress.uploaded << '/' << progress.upload_total
            << ", Download: " << progress.downloaded << '/' << progress.download_total
            << '\n';
    }
});
```

The callback runs synchronously on the thread performing the request and is called once for every progress update from libcurl. Exceptions abort the transfer and are propagated unchanged. When the callback is empty, libcurl progress reporting remains disabled.

## Buffered Response Limit

`max_buffered_response_size` limits every body retained internally by the client. It defaults to 64 MiB and applies equally to successful buffered responses and buffered error responses.

Successful bodies written to an output stream are not affected. Error bodies remain subject to the limit because they are retained for `response_error` even when an output stream was provided.

Set the option to `std::nullopt` to disable the limit:

```cpp
const auto response = http_client.get("https://example.com/large", request_options{
    .max_buffered_response_size = std::nullopt
});
```

The limit counts body bytes after libcurl content decoding. Exceeding it aborts the transfer and throws `error`, because no complete HTTP response is available.
