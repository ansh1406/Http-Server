#ifndef HTTP_INTERNALS_HPP
#define HTTP_INTERNALS_HPP

#include "includes/tcp.hpp"
#include "includes/http_parser.hpp"
#include "includes/http_request.hpp"
#include "includes/http_response.hpp"

#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <functional>

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

        class NotFound : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "404 Not Found";
            }
        };
    }

    class HttpRequestReader
    {
    private:
        std::vector<char> buffer;
        bool validate_request_line(const std::vector<char> &request_line);
        long is_content_length_header(const size_t header_end_index);
        bool is_transfer_encoding_header(const size_t header_end_index);
        void read_from_tcp(tcp::ConnectionSocket &client_socket);

    public:
        HttpRequestReader() = default;
        std::vector<char> read(tcp::ConnectionSocket &client_socket);
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

        void handle(std::map<std::pair<std::string, std::string>,
                             std::function<void(const http::HttpRequest &, http::HttpResponse &)>> &route_handlers);
        void send_response(const http::HttpResponse &response);

        std::string get_ip() const
        {
            return client_socket.get_ip();
        }

        tcp::Port get_port() const
        {
            return client_socket.get_port();
        }
    };
}

#endif // HTTP_INTERNALS_HPP