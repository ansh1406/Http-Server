#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include "tcp.hpp"

#include <string>
#include <map>
#include <vector>
#include <functional>

namespace http
{
    class HttpRequest;
    class HttpResponse;
    
    class HttpConnection
    {
    private:
        std::vector<char> buffer;
        tcp::ConnectionSocket client_socket;
        void read_from_tcp(tcp::ConnectionSocket &client_socket);
        std::vector<char> read(tcp::ConnectionSocket &client_socket);

    public:
        explicit HttpConnection(tcp::ConnectionSocket &&socket)
            : client_socket(std::move(socket)) {}

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

#endif // HTTP_CONNECTION_HPP