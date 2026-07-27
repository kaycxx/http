# http

C++ HTTP client library using libcurl as its backend.

[GitHub] | [API Documentation]

## Requirements

- C++23 compiler and standard library
- libcurl 8.13.0 or newer
- nlohmann/json 3.11.3 or newer
- Exception support

Building the unit tests additionally requires:

- cpp-httplib 0.17.0 or newer
- pkg-config

## Usage

Send a JSON document and parse the JSON response:

```cpp
kaycxx::http::client http_client{};
const auto response = http_client.post("https://example.com/echo", nlohmann::json{
    { "message", "Hello" }
});
const auto message = response.json().at("message").get<std::string>();
```

The guides below cover request bodies, streaming, headers, options, responses, and errors in detail.

CMake users consume the installed package with:

```cmake
find_package(kaycxx-http 0.2.0 CONFIG REQUIRED)
target_link_libraries(my-target PRIVATE kaycxx::http)
```

Non-CMake users can use pkg-config:

```sh
c++ -std=c++23 $(pkg-config --cflags kaycxx-http) -c main.cpp
c++ main.o $(pkg-config --libs kaycxx-http)
```

## Guides

- [Sending Requests] explains client reuse, convenience methods, custom methods, and supported URLs.
- [Request Bodies] explains text, JSON, binary, streamed, and form request bodies together with their ownership rules.
- [Responses and Errors] explains buffered responses, JSON parsing, status handling, and the exception model.
- [Streaming] explains streamed downloads, streamed uploads, bidirectional streaming, and error responses.
- [Headers and Options] explains request and response headers, redirects, timeouts, and response buffer limits.

## Build From Source

```sh
cmake -B build
cmake --build build
```

A shared library is built by default. For a static build:

```sh
cmake -B build -D BUILD_SHARED_LIBS=OFF
cmake --build build
```

## Install

```sh
cmake --install build --prefix /tmp/root
```

If no prefix is specified, CMake installs to `/usr/local` by default on Unix systems.

## Development

Run all tests:

```sh
cmake --build build --target test
```

Generate API documentation with Doxygen:

```sh
cmake --build build --target apidoc
```

The generated HTML documentation is written to `build/apidoc/html/index.html`.

[GitHub]: https://github.com/kaycxx/http
[API Documentation]: https://kaycxx.github.io/http/
[Sending Requests]: docs/sending-requests.md
[Request Bodies]: docs/request-bodies.md
[Responses and Errors]: docs/responses-and-errors.md
[Streaming]: docs/streaming.md
[Headers and Options]: docs/headers-and-options.md
