#ifndef HTTP_HPP
#define HTTP_HPP

#include "includes/tcp.hpp"
#include "includes/http_parser.hpp"

#include <string>
#include <map>
#include <vector>
#include <stdexcept>

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

        class BadRequest : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "400 Bad Request";
            }
        };

        class URITooLong : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "414 URI Too Long";
            }
        };

        class HeaderFieldsTooLarge : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "431 Header Fields Too Large";
            }
        };

        class PayloadTooLarge : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "413 Payload Too Large";
            }
        };

        class InternalServerError : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "500 Internal Server Error";
            }
        };
    }

    class HttpServer
    {
    private:
        tcp::ListeningSocket server_socket;

    public:
        explicit HttpServer(tcp::Port port);

        HttpServer(const HttpServer &) = delete;
        HttpServer &operator=(const HttpServer &) = delete;

        HttpServer(HttpServer &&) = default;
        HttpServer &operator=(HttpServer &&) = default;

        void start();
    };

    class HttpConnection
    {
    private:
        tcp::ConnectionSocket client_socket;
        http::HttpRequestReader request_reader;
        http::HttpRequestParser request_parser;

    public:
        explicit HttpConnection(tcp::ConnectionSocket &&socket)
            : client_socket(std::move(socket)), request_reader(), request_parser() {}

        HttpConnection(const HttpConnection &) = delete;
        HttpConnection &operator=(const HttpConnection &) = delete;

        HttpConnection(HttpConnection &&) = default;
        HttpConnection &operator=(HttpConnection &&) = default;

        void handle();
        void send_response(const http::HttpResponse &response);
    };
}

#endif // HTTP_HPP