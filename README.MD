# Http-Server (C++ HTTP library)

**Stable, minimal, low-level HTTP server library (shared object + header-only API surface).**

This project provides a compact C++ HTTP/1.1 server library intended for low-level system usage. The library exposes a small runtime in `libhttp.so` and three public headers that present the API surface to consumers:

- [includes/http.hpp](includes/http.hpp)
- [includes/http_request.hpp](includes/http_request.hpp)
- [includes/http_response.hpp](includes/http_response.hpp)
- [includes/http_constants.hpp](includes/http_constants.hpp)

The implementation lives in `src/` and the build helper `build.sh` produces `build/libhttp.so` and copies the public headers into `build/` for distribution.

**Intended use**
- Embed a lightweight HTTP server into system services, daemons, or test harnesses.
- Designed for single-process usage where consumer code provides a single global request handler callback for all incoming requests.
- Minimal external dependencies; built with g++ and standard library only.

**Highlights**
- Small, explicit API surface for low-level integration.
- Exposes `HttpServer` runtime via shared object and headers for compile-time types.
- Single global request handler callback — consumers implement routing and response logic.

**Public API (summary)**

- `http::HttpRequest` (in [includes/http_request.hpp](includes/http_request.hpp))
  - Public members: `method`, `uri`, `version`, `headers`, `body`.
  - Represents a parsed incoming HTTP request.

- `http::HttpResponse` (in [includes/http_response.hpp](includes/http_response.hpp))
  - Methods: `version()`, `status_code()`, `status_message()`, `headers()`, `body()` (const accessors).
  - Mutators: `set_status_code(int)`, `set_status_message(string)`, `set_body(vector<char>)`, `add_header(string,string)`.
  - Constructible with just status code / message or with full headers/body.

  - `http::HttpServer` (in [includes/http.hpp](includes/http.hpp))
  - Construct with `http::HttpServerConfig` and a request handler callback:
    - `port` (unsigned short)
    - `max_pending_connections` (unsigned int)
    - `max_concurrent_connections` (unsigned int)
    - `inactive_connection_timeout` (time_t)
    - `external_logging` (bool)
  - Methods:
    - `start()` — begin listening and serving.
  - Construction:
    - The `HttpServer` is constructed with an `HttpServerConfig` and a request handler callback with signature `std::function<void(const http::HttpRequest&, http::HttpResponse&)>`. The provided callback is invoked for every incoming request and should populate the provided `HttpResponse`.
  - Exceptions:
    - `http::exceptions::CanNotCreateServer` — thrown on creation failure.
    - All `http::exceptions` types inherit from `std::runtime_error`.

See the header files for full signatures and inline documentation.

Quick example (consumer code)

```cpp
#include "http.hpp"
#include <iostream>

int main() {
    http::HttpServerConfig cfg{8080, 100, 100, 60, false};
    http::HttpServer server(cfg, [](const http::HttpRequest &req, http::HttpResponse &res){
      res.set_status_code(200);
      res.set_status_message("OK");
      res.add_header("Content-Type", "text/plain");
      std::string body = "Hello, World!";
      res.set_body(std::vector<char>(body.begin(), body.end()));
    });

    server.start();
    return 0;
}
```

Building the library

A convenience script is included: `build.sh`.

From the project root:

```bash
./build.sh
```

`build.sh` will compile the sources into `build/libhttp.so` and copy the public headers into `build/`.

Manual build (equivalent)

```bash
mkdir -p build
g++ -shared -fPIC -I. src/*.cpp -o build/libhttp.so -O2 -std=c++11
cp includes/http.hpp includes/http_request.hpp includes/http_response.hpp includes/http_constants.hpp build/
```

CMake build (recommended)

This repository includes a `CMakeLists.txt` which defines a shared library target named `http`. On Linux the built file will be `libhttp.so` and will be placed in the CMake build directory (`CMAKE_BINARY_DIR`, typically `build/`). The CMake target also performs a post-build step that copies the public headers from `includes/` into the build directory.

Requirements: CMake 3.5 or newer and a C++11-capable compiler.

From the project root:

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The produced shared object will be `build/libhttp.so`. To build a debug variant use `-DCMAKE_BUILD_TYPE=Debug` or pass other generator-specific flags to `cmake`.

The `build.sh` script remains available as a quick single-command build alternative:

```bash
./build.sh
```

Linking against the shared object

Linking against the shared object

Common options:

```bash
g++ -Ibuild -Lbuild -lhttp -Wl,-rpath,$PWD/build my_app.cpp -o my_app

g++ -Ibuild my_app.cpp build/libhttp.so -o my_app
```

Run-time loader notes

- To run an executable that depends on `build/libhttp.so`, either install it to a system library path (`/usr/local/lib`) or set `LD_LIBRARY_PATH`:

```bash
export LD_LIBRARY_PATH=$PWD/build:$LD_LIBRARY_PATH
./my_app
```

API usage notes

- Route handlers are synchronous callbacks invoked by the server runtime. Handlers should be fast and non-blocking where possible.
- The server does not provide high-level routing features (e.g., parameterized routes or middleware). Implement in your consumer code.
- `HttpResponse` body uses `std::vector<char>`; convert from/to `std::string` as needed.