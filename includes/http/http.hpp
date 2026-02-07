#ifndef HTTP_HPP
#define HTTP_HPP

#include "http_request.hpp"
#include "http_response.hpp"

#include <functional>
#include <string>
#include <stdexcept>

namespace http
{
    using RequestHandler = std::function<void(const http::HttpRequest &, http::HttpResponse &)>;
    namespace exceptions
    {
        class CanNotCreateServer : public std::runtime_error
        {
        public:
            CanNotCreateServer(const std::string &message = "")
                : std::runtime_error("HTTP: Unable to create server" + (message.empty() ? "" : "\n" + message)) {}
        };
    }

    struct HttpServerConfig
    {
        unsigned short port;
        unsigned int max_pending_connections;
        unsigned int max_concurrent_connections;
        time_t inactive_connection_timeout;
        bool external_logging;
    };
    class HttpConnection;

    /// @brief A simple HTTP server.
    class HttpServer
    {
    private:
        /// @brief Pointer to incomplete implementation to abstract away underlying details.
        struct Impl;
        Impl *pimpl;

    public:
        /// @brief Opens an HTTP/1.1 on the specified port. Over TCP.
        /// @param config Configuration for the HTTP server.
        /// @throws http::exceptions::CanNotCreateServer if the server cannot be created.
        explicit HttpServer(HttpServerConfig config, RequestHandler handler);

        HttpServer(const HttpServer &) = delete;
        HttpServer &operator=(const HttpServer &) = delete;

        HttpServer(HttpServer &&) = default;
        HttpServer &operator=(HttpServer &&) = default;

        ~HttpServer();

        /// @brief Starts the server to listen for incoming requests.
        void start();
    };
}
#endif // HTTP_HPP

/*
A simple implementation of an HTTP server using this library:
#include "http.hpp"
#include <iostream>
#include <string>
#include <vector>

int main()
{
    try
    {
        http::HttpServerConfig config{8080, 100, 100, 60, false};
        http::HttpServer server(config, [](const http::HttpRequest &req, http::HttpResponse &res) {
            if (req.method == "GET" && req.uri == "/hello") {
                res.set_status_code(200);
                res.set_status_message("OK");
                res.add_header("Content-Type", "text/plain");
                std::string body = "Hello, World!";
                res.set_body(std::vector<char>(body.begin(), body.end()));
            } else {
                res.set_status_code(404);
                res.set_status_message("Not Found");
                res.add_header("Content-Type", "text/plain");
                std::string body = "Not Found";
                res.set_body(std::vector<char>(body.begin(), body.end()));
            }
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
*/