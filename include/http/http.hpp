/*

Http Server is a simple and efficient HTTP server library implemented in C++.
It provides a straightforward interface for handling HTTP requests and responses, making it easy to create web servers and APIs.
The library supports HTTP/1.1 and includes features such as request parsing, response generation, and inbuilt connection management.
With its modular design, developers can easily extend its functionality to suit their specific needs.

*/

///@file http.hpp
///@brief This file includes all the necessary headers for using the HTTP server library. It provides a convenient way to include all the components of the library with a single include statement.

#ifndef HTTP_HPP
#define HTTP_HPP

#include "http_request.hpp"
#include "http_response.hpp"
#include "http_constants.hpp"

#include <functional>
#include <string>
#include <stdexcept>


/// @brief Namespace for the HTTP server library. All the classes, functions, and constants related to the HTTP server are defined within this namespace.
namespace http
{
    /// @brief Type alias for the request handler function. It is a std::function that takes a const reference to an HttpRequest and a non-const reference to an HttpResponse, and returns void. This function will be called for each incoming HTTP request, allowing the user to process the request and generate an appropriate response.
    using RequestHandler = std::function<void(const http::HttpRequest &, http::HttpResponse &)>;

    namespace exceptions
    {
        /// @brief Exception class for errors that occur when the server cannot be created. It inherits from std::runtime_error and provides a constructor that takes an optional message to provide more details about the error.
        class CanNotCreateServer : public std::runtime_error
        {
        public:
            CanNotCreateServer(const std::string &message = "")
                : std::runtime_error("HTTP: Unable to create server" + (message.empty() ? "" : "\n" + message)) {}
        };
    }

    /// @brief Configuration structure for the HTTP server. It contains various parameters that can be set to configure the behavior of the server, such as the port to listen on, maximum pending connections, maximum concurrent connections, timeout for inactive connections, and whether to enable external logging.
    struct HttpServerConfig
    {
        /// @brief The port number on which the HTTP server will listen for incoming connections. It is an unsigned short integer, and the default value is typically 80 for HTTP.
        unsigned short port;
        /// @brief The maximum number of pending connections that the server can have in its queue. This parameter controls how many incoming connections can be waiting to be accepted before the server starts rejecting new connections. It is an unsigned integer.
        unsigned int max_pending_connections;
        /// @brief The maximum number of concurrent connections that the server can handle at any given time. It is an unsigned integer. If the number of active connections exceeds this limit, the server may start rejecting new connections until some of the existing connections are closed.
        unsigned int max_concurrent_connections;
        /// @brief The timeout duration in seconds for inactive connections. If a connection remains idle (i.e., no data is sent or received) for longer than this duration, the server may close the connection to free up resources. It is a time_t value.
        time_t inactive_connection_timeout_in_seconds;
        /// @brief A boolean flag indicating whether to enable external logging. If set to true, the server will log information about incoming requests, responses, and other events to an external logging system. If set to false, the server will log to stdout and stderr if external logging is disabled. The default value is false.
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
        /// @param handler The request handler function that will be called for each incoming HTTP request. It should be a function that takes a const reference to an HttpRequest and a non-const reference to an HttpResponse, and returns void. This function will be responsible for processing the incoming requests and generating appropriate responses.
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