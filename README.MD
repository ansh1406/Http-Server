# Simple C++ HTTP Server Library

This project provides a simple, robust, and extensible HTTP/1.1 server library written in C++. It is designed for ease of use, allowing you to quickly create HTTP servers, define custom route handlers, and manage HTTP requests and responses with a clean API.

## Features
- **HTTP/1.1 server**: Listens for incoming HTTP connections and serves requests over TCP.
- **Route handling**: Register custom handlers for specific HTTP methods and URL paths.

## Directory Structure
- `includes/`: Public headers for the HTTP server, request, response, parser, and TCP layer.
- `src/`: Implementation files for the HTTP server and TCP networking.
- `build/`: Output directory for compiled library and copied headers.
- `build.sh`: Bash script to build the shared library and copy headers.

## Build Instructions

**Requirements:**
- Linux or WSL (for bash and POSIX sockets)
- g++ (C++11 or later)
- bash

**To build the library:**

```sh
./build.sh
```

This will compile the source files and produce `build/http.so` (a shared library), and copy the main headers to the `build/` directory.

## Example Usage

Below is a minimal example of how to use this library to create a simple HTTP server that responds to `GET /hello`:

```cpp
#include "http.hpp"
#include <iostream>
#include <string>
#include <vector>

int main()
{
    try
    {
        http::HttpServerConfig config{port: 8080, max_pending_connections: 10};
        http::HttpServer server(config);

        server.add_route_handler("GET", "/hello", [](const http::HttpRequest &req, http::HttpResponse &res) {
            res.set_status_code(200);
            res.set_status_message("OK");
            res.add_header("Content-Type", "text/plain");
            std::string body = "Hello, World!";
            res.set_body(std::vector<char>(body.begin(), body.end()));
        });

        server.start();
    }
    catch (const http::exceptions::CanNotCreateServer &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cerr << "An unexpected error occurred." << std::endl;
        return 1;
    }
    return 0;
}
```

Compile your program with:

```sh
g++ -Ipath_to_http.hpp your_program.cpp http.so -o your_program
```

Run your server:

```sh
./your_program
```

## API Overview

### Main Classes
- **`http::HttpServer`**: Manages the server lifecycle, route registration, and request handling.
	- `HttpServer(unsigned short port)`: Create a server listening on the given port.
	- `void start()`: Start accepting connections and handling requests.
	- `void add_route_handler(const std::string &method, const std::string &path, std::function<void(const HttpRequest&, HttpResponse&)> handler)`: Register a handler for a method/path.
- **`http::HttpRequest`**: Represents an HTTP request.
	- `const std::string &method() const`
	- `const std::string &uri() const`
	- `const std::map<std::string, std::string> &headers() const`
	- `const std::vector<char> &body() const`
- **`http::HttpResponse`**: Represents an HTTP response.
	- `void set_status_code(int)`
	- `void set_status_message(const std::string &)`
	- `void add_header(const std::string &, const std::string &)`
	- `void set_body(const std::vector<char> &)`
