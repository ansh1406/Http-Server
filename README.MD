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

## Usage

### Classes

- `HttpServer`: The main class for creating and managing the HTTP server.
- `HttpServerConfig`: A configuration class for setting up the server parameters such as port number, maximum connections, and timeout settings.
- `HttpRequest`: Represents an incoming HTTP request, providing access to headers, body, and other request details.
- `HttpResponse`: Represents an HTTP response that can be sent back to the client, allowing you to set status codes, headers, and body content.

> Note: All public API functions and classes are declared under the `http` namespace. All functions are well documented in the include files.

### How to use in your project

To use the Http-Server library in your C++ project, follow these steps:

1. Clone the repository and build the library.

2. Add [include](include) folder to your project's include path. (If you are copying the header files directly into your project, you are suggested to copy the entire [http](include/http) folder to your project as it is to avoid any issues with the include paths.)

3. Link against the compiled library when building your project.

### Example

Below is a simple example of how to create a basic HTTP server that responds with "Hello, World!" to any incoming request.

```cpp
#include "http/http.hpp"

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
    });

    server.start();
  }
  catch(const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}
```

## Build Instructions

> [!WARNING]
> The library is currently only supported on Posix complient linux systems.

We have provided 2 methods to build the library. You can choose either of them based on your preference.

### Method 1: Using CMake

1. Clone the repository:
   ```bash
   git clone "https://github.com/ansh1406/Http-Server.git"
   ```
2. Navigate to the project directory and create a build directory:
   ```bash
   cd Http-Server
   mkdir build
   cd build
   ```
3. Run CMake to configure the project and generate the build files:
   ```bash
   cmake ..
   ```
4. Build the library using the generated build files:
   ```bash
   cmake --build .
   ```
5. This will output `libhttp.so` or `libhttp.a` in the `build` directory according to the `BUILD_LIBRARY_TYPE` option in `CMakeLists.txt`. (This will also output object files as target `http_objects` regardless of any `BUILD_LIBRARY_TYPE` option. You can link against these object files instead of the library if needed.)

### Method 2: Using Bash Script

1. Clone the repository:

   ```bash
   git clone "https://github.com/ansh1406/Http-Server.git"

   ```

2. Navigate to the project directory and run the build script:
   ```bash
   cd Http-Server
   sh ./build.sh --help
   ```
   then proceed with the instructions provided by the script.

> This method is not recommended but can be used if you don't have CMake installed or prefer a simpler build process.
