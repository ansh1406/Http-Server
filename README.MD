# Http-Server (HTTP library for C++)

## Overview

Http-Server is a C++ library designed to facilitate the creation of HTTP servers. It provides a simple and efficient way to handle HTTP requests and responses, making it easier for developers to build web applications and services.

## Features

- Support for HTTP/1.1 protocol.
- Event driven architecture for handling multiple connections concurrently.
- Easy-to-use API for processing HTTP requests and generating responses.
- Built-in logging functionality for monitoring server activity and debugging.
- Support for reading chunked bodies.
- Automatic handling of connection timeouts and protocol level errors.
- Automatic setting of Content-Length header for responses with a body.

## Build Instructions

You can build the library using either CMake (recommended) or the provided build script.

### Method 1: Using CMake

```bash
cmake -S . -B build
cmake --build build
```

This outputs the `http` library in the `build` directory according to the `BUILD_LIBRARY_TYPE` option in `CMakeLists.txt` (for example, static or shared based on your platform and toolchain).
It also outputs object files as target `http_objects` regardless of `BUILD_LIBRARY_TYPE`.

### Method 2: Using Bash Script

```bash
sh ./build.sh --help
```

Then proceed with the options provided by the script.

> This method can be used if you prefer a script-driven build.

## Installation

If you built using CMake, install with:

```bash
cmake --install build
```

For a custom installation path:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/your/custom/path
cmake --build build
cmake --install build
```

## Usage

### Classes

- `HttpServer`: The main class for creating and managing the HTTP server.
- `HttpServerConfig`: A configuration class for setting up the server parameters such as port number, maximum connections, and timeout settings.
- `HttpRequest`: Represents an incoming HTTP request, providing access to headers, body, and other request details.
- `HttpResponse`: Represents an HTTP response that can be sent back to the client, allowing you to set status codes, headers, and body content.

> Note: All public API functions and classes are declared under the `http` namespace. All functions are well documented in the include files.

### How to use in your project

1. Build and install the library.

2. Link the package with CMake:

```cmake
find_package(http REQUIRED)
target_link_libraries(your_target_name PRIVATE http::http)
```

3. Include headers in your source files:

```cpp
#include "http/http.hpp"
```

### Example

Below is a simple example of how to create a basic HTTP server that responds with "Hello, World!" to any incoming request.

```cpp
#include "http/http.hpp"

#include <iostream>
#include <vector>
#include <string>

int main() {
  try
  {
    http::HttpServerConfig config;
    config.port = 8080;

    http::HttpServer server(config,[](http::HttpRequest &request, http::HttpResponse &response) {
    response.set_status_code(200);
    response.add_header("Content-Type", "text/plain");
    std::string body = "Hello, World!";
    response.set_body(std::vector<char>(body.begin(), body.end()));
    response.add_header("Content-Length", std::to_string(body.size()));
    });

    server.start();
  }
  catch(const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}
```
