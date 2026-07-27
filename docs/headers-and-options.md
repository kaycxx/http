# Headers and Options

`request_options` is an aggregate intended for designated initialization. Options passed to the `client` constructor become defaults for every request sent by that instance. Request-specific values override corresponding client defaults, while unset values inherit them.

```cpp
#include <kaycxx/http.hpp>

using namespace kaycxx::http;

int main() {
    client http_client{{
        .headers = {
            { "User-Agent", "example-client/1.0" },
            { "Accept", "application/json" }
        },
        .follow_redirects = true,
        .max_redirects = 5,
        .connect_timeout = std::chrono::seconds{5},
        .request_timeout = std::chrono::minutes{1},
        .max_buffered_response_size = 128 * 1024 * 1024
    }};

    const auto response = http_client.get("https://example.com/data", {
        .headers = {
            { "X-Request-Id", "example-42" }
        }
    });

    return response.status_code() == 200 ? 0 : 1;
}
```

When neither the request nor the client configures an option, the library or libcurl default applies. Client defaults should only contain settings appropriate for every request sent by that instance. Client headers and progress callbacks cannot be removed for an individual request; configure them per request when they are not universally applicable.

## Request Headers

Request header names and values are owning `std::string` values. A header with an empty value is sent as a present header with no value.

Client and request headers are combined. A request header replaces all client headers with the same ASCII case-insensitive name, while client headers with other names are retained.

```cpp
const auto response = http_client.get("https://example.com/data", {
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
const auto response = http_client.get("https://example.com/redirect", {
    .follow_redirects = true,
    .max_redirects = 10
});
```

Both redirect options are optional. An unset request option inherits its client default. When neither level configures the option, nothing is sent to libcurl, which disables redirect following and uses a limit of 30 redirects if following is enabled. An explicitly configured `max_redirects` is sent independently of `follow_redirects`, although it only has an effect when redirect following is enabled.

Redirect status codes select the subsequent method according to normal libcurl rules. A 303 switches to GET except after HEAD. A 307 or 308 retains the method and request body. POST requests normally switch to GET after 301 or 302.

Custom request headers are reused after redirects and can therefore be sent to a different origin. libcurl withholds `Authorization` and explicitly configured `Cookie` headers after an origin change, but arbitrary sensitive headers receive no such protection. Enable redirects with care when options contain credentials in custom header names.

## Timeouts

`connect_timeout` limits the connection phase. `request_timeout` limits the complete transfer, including the connection phase. Both are optional `std::chrono::milliseconds` values and accept compatible chrono durations.

An unset request timeout inherits its client default. When neither level configures a timeout, the option is not sent to libcurl. Its built-in defaults therefore apply: 300 seconds for the connection phase and no timeout for the complete transfer.

Timeout failures throw `error`.

## Transfer Progress

`progress` receives the current upload and download progress reported by libcurl. `download_total` and `upload_total` contain the expected total number of bytes. A value of `0` means that the total is unknown, the direction is unused or the transfer is empty; libcurl does not distinguish these cases in progress reports.

```cpp
const auto response = http_client.put("https://example.com/data", input, output, {
    .progress = [](const transfer_progress& progress) {
        std::cout
            << "Upload: " << progress.uploaded << '/' << progress.upload_total
            << ", Download: " << progress.downloaded << '/' << progress.download_total
            << '\n';
    }
});
```

The callback runs synchronously on the thread performing the request and is called once for every progress update from libcurl. Exceptions abort the transfer and are propagated unchanged. An empty request callback inherits the client callback. When neither level supplies a callback, libcurl progress reporting remains disabled. A request callback replaces the client callback for that transfer.

## Buffered Response Limit

`max_buffered_response_size` limits every body retained internally by the client and applies equally to successful buffered responses and buffered error responses. An unset request value inherits the client setting. When neither level configures a value, the library uses 64 MiB.

Successful bodies written to an output stream are not affected. Error bodies remain subject to the limit because they are retained for `response_error` even when an output stream was provided.

Set the option to zero to disable the limit:

```cpp
const auto response = http_client.get("https://example.com/large", {
    .max_buffered_response_size = 0
});
```

The limit counts body bytes after libcurl content decoding. Exceeding it aborts the transfer and throws `error`, because no complete HTTP response is available.
