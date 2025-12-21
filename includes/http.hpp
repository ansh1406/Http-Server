#ifndef HTTP_HPP
#define HTTP_HPP

#include "http_constants.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <map>
#include <functional>
#include <string>
#include <exception>

namespace http
{
    namespace exceptions
    {
        class CanNotCreateServer : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "Unable to create server.";
            }
        };
    }

    /// @brief A simple HTTP server.
    class HttpServer
    {
    private:
        /// @brief Pointer to incomplete implementation to abstract away tcp layer details.
        /// @param server_socket Holds the tcp::ListeningSocket.
        struct Impl;
        /// @brief Pointer to the implementation.
        Impl *pimpl;
        std::map<std::pair<std::string, std::string>, std::function<void(const http::HttpRequest &, http::HttpResponse &)>> route_handlers;

    public:
        /// @brief Opens an HTTP/1.1 on the specified port. Over TCP.
        /// @param port Port number to listen on.
        /// @throws http::exceptions::CanNotCreateServer if the server cannot be created.
        explicit HttpServer(unsigned short port);

        HttpServer(const HttpServer &) = delete;
        HttpServer &operator=(const HttpServer &) = delete;

        HttpServer(HttpServer &&) = default;
        HttpServer &operator=(HttpServer &&) = default;

        ~HttpServer();

        /// @brief Starts the server to listen for incoming requests.
        void start();
        /// @brief Adds a route handler for the specified HTTP method and path.
        /// @param method Http method (e.g., "GET", "POST"). Method names are case-sensitive.
        /// @param path URL path (e.g., "/api/data"). Path is case-sensitive.
        /// @param handler Accepts a callback function that takes an http::HttpRequest and http::HttpResponse as parameters.
        void add_route_handler(const std::string method, const std::string path,
                               const std::function<void(const http::HttpRequest &, http::HttpResponse &)> handler);
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
        http::HttpServer server(8080);

        server.add_route_handler("GET", "/hello", [](const http::HttpRequest &req, http::HttpResponse &res) {
            res.set_status_code(200);
            res.set_status_message("OK");
            res.add_header("Content-Type", "text/plain");
            res.add_header("Connection", "close");
            res.add_header("Content-Length", "13");
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
*/