# Http-Server

## Overview

Http-Server is a low-level C++ HTTP/1.1 server library. It exposes a small API under the `http` namespace for request parsing, response generation, connection management, and request and response body handling.

## Contract

### What it does

- Accepts HTTP/1.1 requests.
- Parses request lines, headers, and request bodies.
- Builds responses with headers and body data.
- Manages connections and request handling in an event-driven loop.

### What it does not do

- It is not a routing framework.
- It does not provide middleware, templates, or session management.
- It does not hide HTTP details from the caller.

## Defaults

| Setting | Default |
| --- | --- |
| Port | `8080` |
| Pending connections | `128` |
| Concurrent connections | `128` |
| Idle timeout | `60` seconds |
| Logging | Disabled |

## Limits

- Request line limit: 8 KB.
- Header block limit: 8 KB.
- Read buffer size: 8 KB.
- Response version is fixed to HTTP/1.1.

## Ownership and Lifetime

- `HttpRequest` is read-only from user code.
- `HttpResponse` is mutated by the request handler.
- Request and response body handling is stateful; repeated reads continue from the current position.
- Moving a request or response must preserve stream state in the destination object.

## Thread Safety

- Treat request and response objects as single-threaded, per-request objects.
- Do not share stream objects across threads unless you own the synchronization.
- Logging is synchronized internally.

## Error Model

- Most protocol and stream failures are reported through exceptions.
- Invalid requests, malformed headers, and unsupported protocol states fail fast.
- Body-related errors are surfaced through library exceptions.

## Build Instructions

You can build the library using either CMake or the provided script.

### CMake

```bash
cmake -S . -B build
cmake --build build
```

### Script

```bash
sh ./build.sh --help
```

## Install

```bash
cmake --install build
```

For a custom install prefix:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/your/custom/path
cmake --build build
cmake --install build
```

## Using the Library

### CMake

```cmake
find_package(http REQUIRED)
target_link_libraries(your_target_name PRIVATE http::http)
```

### Include

```cpp
#include "http/http.hpp"
```

### Core Types

- `HttpServer`: server entry point.
- `HttpServerConfig`: server settings.
- `HttpRequest`: incoming request view.
- `HttpResponse`: outgoing response object.

## Example

```cpp
#include "http/http.hpp"

#include <iostream>
#include <string>
#include <vector>

int main()
{
    try
    {
        http::HttpServerConfig config;
        config.port = 8080;

        http::HttpServer server(config, [](const http::HttpRequest &, http::HttpResponse &response) {
            response.set_status_code(http::status_codes::OK);
            response.set_status_message("OK");
            response.set_header("Content-Type", "text/plain");

            std::string body = "Hello, World!";
            response.set_body(std::vector<char>(body.begin(), body.end()));
        });

        server.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
```

## Notes

- Public headers are in `include/http/`.
- The exported CMake target is `http::http`.
- The library uses platform-specific socket/event backends under the hood.
