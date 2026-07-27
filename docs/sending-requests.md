# Sending Requests

`client` is a synchronous HTTP client. Keep one instance for a sequence of requests so its libcurl backend can reuse connections.

```cpp
#include <kaycxx/http.hpp>

using namespace kaycxx::http;

int main() {
    client http_client{};
    const auto response = http_client.get("https://example.com/data");

    return response.status_code() == 200 ? 0 : 1;
}
```

A client is movable but not copyable. A single instance must not be used concurrently from multiple threads. Separate client instances can be used by separate threads. Starting another request reentrantly on the same instance, such as from a progress callback, throws `error`.

Options passed to the constructor become defaults for all requests sent by that client. Request-specific options override corresponding values and merge headers by ASCII case-insensitive name:

```cpp
client http_client{{
    .headers = {
        { "User-Agent", "example-client/1.0" }
    },
    .request_timeout = std::chrono::seconds{30}
}};

const auto response = http_client.get("https://example.com/data", {
    .request_timeout = std::chrono::seconds{5}
});
```

[Headers and Options] describes inheritance, header merging, and all available settings.

## Convenience Methods

The convenience methods select the common HTTP methods without passing a method string explicitly:

- `get()` sends `GET`.
- `post()` sends `POST`.
- `put()` sends `PUT`.
- `del()` sends `DELETE`.

Each method supports the same four request shapes:

| Request body | Response body | Example |
| --- | --- | --- |
| None | Buffered | `http_client.get(url)` |
| Present | Buffered | `http_client.post(url, body)` |
| None | Streamed | `http_client.get(url, output)` |
| Present | Streamed | `http_client.put(url, body, output)` |

[Request Bodies] and [Streaming] describe these overloads in detail.

All methods accept `request_options` as their final argument. See [Headers and Options] for the available settings and their defaults.

```cpp
const auto response = http_client.get("https://example.com/data", {
    .headers = {
        { "Accept", "application/json" }
    }
});
```

Every status code from 200 through 299 is treated as success. Other status codes throw `response_error`. Transport, configuration, stream, and buffering failures throw `error`. [Responses and Errors] describes both result paths.

## Custom Methods

Use `request()` for methods without a dedicated convenience function. The method string is passed through to libcurl without a fixed whitelist.

```cpp
const auto response = http_client.request(
    "PROPFIND",
    "https://example.com/documents",
    {
        .headers = {
            { "Depth", "1" }
        }
    }
);
```

Custom methods support the same body, streaming, and options overloads as the convenience methods.

## Supported URLs

Only `http` and `https` URLs are allowed, including redirect targets. TLS certificate and hostname verification use libcurl's secure defaults.

The client asks libcurl to negotiate and decode its supported HTTP content encodings automatically. `buffered_response::text()` exposes the decoded body bytes; it does not perform character-set conversion.

[Request Bodies]: request-bodies.md
[Responses and Errors]: responses-and-errors.md
[Streaming]: streaming.md
[Headers and Options]: headers-and-options.md
