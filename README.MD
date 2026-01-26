# Http-Server (C++ HTTP library)

**Stable, minimal, low-level HTTP server library (shared object + header-only API surface).**

This project provides a compact C++ HTTP/1.1 server library intended for low-level system usage. The library exposes a small runtime in `http.so` and three public headers that present the API surface to consumers:

- [includes/http.hpp](includes/http.hpp)
- [includes/http_request.hpp](includes/http_request.hpp)
- [includes/http_response.hpp](includes/http_response.hpp)
- [includes/http_constants.hpp](includes/http_constants.hpp)

The implementation lives in `src/` and the build helper `build.sh` produces `build/http.so` and copies the public headers into `build/` for distribution.

**Intended use**
- Embed a lightweight HTTP server into system services, daemons, or test harnesses.
- Designed for single-process, non-opinionated routing: user code provides route handlers.
- Minimal external dependencies; built with g++ and standard library only.

**Highlights**
- Small, explicit API surface for low-level integration.
- Exposes `HttpServer` runtime via shared object and headers for compile-time types.
- Simple route handler registration based on method+path.

**Public API (summary)**

- `http::HttpRequest` (in [includes/http_request.hpp](includes/http_request.hpp))
  - Public members: `method`, `uri`, `version`, `headers`, `body`.
  - Represents a parsed incoming HTTP request.

- `http::HttpResponse` (in [includes/http_response.hpp](includes/http_response.hpp))
  - Methods: `version()`, `status_code()`, `status_message()`, `headers()`, `body()` (const accessors).
  - Mutators: `set_status_code(int)`, `set_status_message(string)`, `set_body(vector<char>)`, `add_header(string,string)`.
  - Constructible with just status code / message or with full headers/body.

- `http::HttpServer` (in [includes/http.hpp](includes/http.hpp))
  - Construct with `http::HttpServerConfig`:
    - `port` (unsigned short)
    - `max_pending_connections` (unsigned int)
    - `max_concurrent_connections` (unsigned int)
    - `inactive_connection_timeout` (time_t)
    - `external_logging` (bool)
  - Methods:
    - `start()` — begin listening and serving.
    - `add_route_handler(method, path, handler)` — register a route handler where `handler` is `std::function<void(const http::HttpRequest&, http::HttpResponse&)>`.
  - Exception: `http::exceptions::CanNotCreateServer` thrown on creation failure.

See the header files for full signatures and inline documentation.

Quick example (consumer code)

```cpp
#include "http.hpp"
#include <iostream>

int main() {
    http::HttpServerConfig cfg{8080, 100, 100, 60, false};
    http::HttpServer server(cfg);

    server.add_route_handler("GET", "/hello", [](const http::HttpRequest &req, http::HttpResponse &res){
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

`build.sh` will compile the sources into `build/http.so` and copy the public headers into `build/`.

Manual build (equivalent)

```bash
mkdir -p build
g++ -shared -fPIC -I. src/*.cpp -o build/http.so -O2 -std=c++11
cp includes/http.hpp includes/http_request.hpp includes/http_response.hpp includes/http_constants.hpp build/
```

Linking against the shared object

Two common options:

- Rename the produced file to follow the platform convention and link normally:

```bash
mv build/http.so build/libhttp.so
g++ -Ibuild -Lbuild -lhttp -Wl,-rpath,$PWD/build my_app.cpp -o my_app
```

- Keep the name `http.so` and link with the linker `-l:` syntax:

```bash
g++ -Ibuild -Lbuild -l:http.so -Wl,-rpath,$PWD/build my_app.cpp -o my_app
```

Or link by absolute path at link time:

```bash
g++ -Ibuild my_app.cpp build/http.so -o my_app
```

Run-time loader notes

- To run an executable that depends on `build/http.so`, either install it to a system library path (`/usr/local/lib`) or set `LD_LIBRARY_PATH`:

```bash
export LD_LIBRARY_PATH=$PWD/build:$LD_LIBRARY_PATH
./my_app
```

API usage notes

- Route handlers are synchronous callbacks invoked by the server runtime. Handlers should be fast and non-blocking where possible.
- The server does not provide high-level routing features (e.g., parameterized routes or middleware). Implement in your consumer code.
- `HttpResponse` body uses `std::vector<char>`; convert from/to `std::string` as needed.